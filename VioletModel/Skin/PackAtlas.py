#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将当前目录下的所有图像打包成一张图集 (texture atlas)，并生成 JSON 描述每个子图的位置。

设计目标（按优先级）：
  1. 空间利用率大于一切  —— 使用 MaxRects (BSSF) 装箱算法 + 尺寸搜索，逼近最优紧密排布。
  2. 子图不可旋转        —— 装箱时从不旋转任何子图。
  3. 内容必须完整保留    —— 默认整图打包，像素零丢失；
                            可选 --trim 裁掉透明边缘，此时在 JSON 记录原始尺寸与偏移，仍可完美还原。

用法:
    python PackAtlas.py [选项]

选项:
    -o, --output      输出图集文件名           (默认: Atlas.png)
    -j, --json        输出 JSON 文件名          (默认: Atlas.json)
    -p, --padding     子图之间的间距(像素)      (默认: 0)
    -e, --extrude     边缘扩边(像素,防止采样渗色)(默认: 0)
    --trim            裁剪透明边缘             (默认: 关闭，整图打包)
    --pow2            图集边长强制为 2 的幂
    --square          图集强制为正方形
    --max-size        图集最大边长              (默认: 8192)
    --dir             要打包的目录              (默认: 当前目录)

.PackIgnore:
    若打包目录下存在 .PackIgnore，其中每行指定一个要忽略的文件名或 glob 通配
    (如 *.ico、Default*.png)，'#' 开头的行视为注释。
"""

import os
import sys
import json
import fnmatch
import argparse

try:
    from PIL import Image
except ImportError:
    sys.exit("需要 Pillow: 请运行  pip install Pillow")

IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".tga", ".webp", ".ico"}
IGNORE_FILE = ".PackIgnore"


# --------------------------------------------------------------------------- #
# MaxRects 装箱算法 (Best Short Side Fit)，不旋转
# --------------------------------------------------------------------------- #
class MaxRectsBin:
    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.free_rects = [(0, 0, width, height)]  # (x, y, w, h)

    def insert(self, w, h):
        best = self._find_position(w, h)
        if best is None:
            return None
        x, y = best
        self._place((x, y, w, h))
        return x, y

    def _find_position(self, w, h):
        best_short = None
        best_long = None
        best_pos = None
        for (fx, fy, fw, fh) in self.free_rects:
            if fw >= w and fh >= h:
                leftover_h = fw - w
                leftover_v = fh - h
                short = min(leftover_h, leftover_v)
                long = max(leftover_h, leftover_v)
                if best_pos is None or short < best_short or (short == best_short and long < best_long):
                    best_short = short
                    best_long = long
                    best_pos = (fx, fy)
        return best_pos

    def _place(self, rect):
        rx, ry, rw, rh = rect
        new_free = []
        for fr in self.free_rects:
            if self._overlap(fr, rect):
                new_free.extend(self._split(fr, rect))
            else:
                new_free.append(fr)
        # 去掉被其它自由矩形完全包含的冗余矩形
        self.free_rects = self._prune(new_free)

    @staticmethod
    def _overlap(a, b):
        ax, ay, aw, ah = a
        bx, by, bw, bh = b
        return not (bx >= ax + aw or bx + bw <= ax or by >= ay + ah or by + bh <= ay)

    @staticmethod
    def _split(free, used):
        fx, fy, fw, fh = free
        ux, uy, uw, uh = used
        pieces = []
        # 上
        if uy > fy:
            pieces.append((fx, fy, fw, uy - fy))
        # 下
        if uy + uh < fy + fh:
            pieces.append((fx, uy + uh, fw, (fy + fh) - (uy + uh)))
        # 左
        if ux > fx:
            pieces.append((fx, fy, ux - fx, fh))
        # 右
        if ux + uw < fx + fw:
            pieces.append((ux + uw, fy, (fx + fw) - (ux + uw), fh))
        return pieces

    @staticmethod
    def _contained(a, b):
        # a 是否完全包含于 b
        ax, ay, aw, ah = a
        bx, by, bw, bh = b
        return ax >= bx and ay >= by and ax + aw <= bx + bw and ay + ah <= by + bh

    def _prune(self, rects):
        pruned = []
        for i, r in enumerate(rects):
            if r[2] <= 0 or r[3] <= 0:
                continue
            if any(j != i and self._contained(r, other) for j, other in enumerate(rects)):
                continue
            pruned.append(r)
        return pruned


# --------------------------------------------------------------------------- #
# 忽略规则 (.PackIgnore)
# --------------------------------------------------------------------------- #
def load_ignore(directory):
    """读取 .PackIgnore，返回 glob 模式列表。"""
    patterns = []
    path = os.path.join(directory, IGNORE_FILE)
    if os.path.isfile(path):
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                patterns.append(line)
    return patterns


def is_ignored(fname, patterns):
    return any(fnmatch.fnmatch(fname, p) for p in patterns)


# --------------------------------------------------------------------------- #
# 载入与预处理
# --------------------------------------------------------------------------- #
def load_sprites(directory, exclude, ignore_patterns, trim):
    """返回子图列表: dict(name, image, ow, oh, ox, oy, w, h)"""
    sprites = []
    for fname in sorted(os.listdir(directory)):
        path = os.path.join(directory, fname)
        if not os.path.isfile(path):
            continue
        ext = os.path.splitext(fname)[1].lower()
        if ext not in IMAGE_EXTS or fname in exclude:
            continue
        if is_ignored(fname, ignore_patterns):
            print(f"  忽略 {fname} (.PackIgnore)")
            continue
        try:
            img = Image.open(path).convert("RGBA")
        except Exception as e:
            print(f"  跳过 {fname}: {e}")
            continue

        ow, oh = img.size
        ox, oy = 0, 0
        cropped = img
        if trim:
            bbox = img.getbbox()  # 完全透明像素的裁剪框
            if bbox is None:
                # 整张透明，保留 1x1 以免丢失
                bbox = (0, 0, 1, 1)
            ox, oy = bbox[0], bbox[1]
            cropped = img.crop(bbox)

        w, h = cropped.size
        sprites.append({
            "name": fname,
            "image": cropped,
            "orig_w": ow, "orig_h": oh,   # 原始完整尺寸
            "offset_x": ox, "offset_y": oy,  # 内容在原图中的偏移
            "w": w, "h": h,               # 打包所用尺寸
        })
    return sprites


# --------------------------------------------------------------------------- #
# 尺寸搜索：找到能装下所有子图的最小图集
# --------------------------------------------------------------------------- #
def next_pow2(n):
    p = 1
    while p < n:
        p <<= 1
    return p


def try_pack(sprites, width, height, padding):
    """尝试把所有子图装进 width x height。成功返回 placements，否则 None。"""
    bin_ = MaxRectsBin(width, height)
    # 面积从大到小排序有利于 MaxRects 收敛
    order = sorted(range(len(sprites)),
                   key=lambda i: max(sprites[i]["w"], sprites[i]["h"]),
                   reverse=True)
    placements = {}
    for i in order:
        s = sprites[i]
        pos = bin_.insert(s["w"] + padding, s["h"] + padding)
        if pos is None:
            return None
        placements[i] = (pos[0], pos[1])
    return placements


def search_size(sprites, padding, max_size, pow2, square):
    total_area = sum((s["w"] + padding) * (s["h"] + padding) for s in sprites)
    max_w = max(s["w"] + padding for s in sprites)
    max_h = max(s["h"] + padding for s in sprites)

    # 以理论下界(总面积)为起点，逐步放大直到装下
    start = max(int(total_area ** 0.5), max_w, max_h)

    candidates = []
    size = start
    while size <= max_size * 2:
        w = h = size
        if pow2:
            w = h = next_pow2(size)
        if not square:
            # 允许非正方形：尝试几种宽高比
            candidates.append((w, h))
            candidates.append((w, max(max_h, (total_area // w) + max_h)))
        else:
            candidates.append((w, h))
        size = int(size * 1.1) + 1
        if len(candidates) > 60:
            break

    # 逐个候选尝试，返回第一个成功且面积最小的
    best = None
    for (w, h) in sorted(set(
            (min(a, max_size), min(b, max_size)) for a, b in candidates), key=lambda t: t[0] * t[1]):
        if w < max_w or h < max_h:
            continue
        placements = try_pack(sprites, w, h, padding)
        if placements is not None:
            best = (w, h, placements)
            break

    # 若都失败，直接用最大尺寸兜底再试一次
    if best is None:
        w = h = max_size
        if pow2:
            w = h = next_pow2(max_size)
        placements = try_pack(sprites, w, h, padding)
        if placements is None:
            sys.exit(f"错误: 即使 {w}x{h} 也无法装下所有子图，请调大 --max-size。")
        best = (w, h, placements)

    # 用实际占用范围收紧最终画布尺寸
    w, h, placements = best
    used_r = max(placements[i][0] + sprites[i]["w"] for i in placements) + padding
    used_b = max(placements[i][1] + sprites[i]["h"] for i in placements) + padding
    fw, fh = used_r, used_b
    if pow2:
        fw, fh = next_pow2(used_r), next_pow2(used_b)
    if square:
        side = max(fw, fh)
        fw = fh = side
    return fw, fh, placements


# --------------------------------------------------------------------------- #
# 渲染图集
# --------------------------------------------------------------------------- #
def render_atlas(sprites, placements, width, height, extrude):
    atlas = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    for i, (x, y) in placements.items():
        img = sprites[i]["image"]
        atlas.paste(img, (x, y))
        if extrude > 0:
            _extrude(atlas, img, x, y, extrude)
    return atlas


def _extrude(atlas, img, x, y, e):
    w, h = img.size
    # 四边扩展
    left = img.crop((0, 0, 1, h))
    right = img.crop((w - 1, 0, w, h))
    top = img.crop((0, 0, w, 1))
    bottom = img.crop((0, h - 1, w, h))
    for k in range(1, e + 1):
        atlas.paste(left, (x - k, y))
        atlas.paste(right, (x + w - 1 + k, y))
        atlas.paste(top, (x, y - k))
        atlas.paste(bottom, (x, y + h - 1 + k))


# --------------------------------------------------------------------------- #

def main():
    ap = argparse.ArgumentParser(description="将目录内所有图像打包成图集 + JSON")
    ap.add_argument("-o", "--output", default="Atlas.png")
    ap.add_argument("-j", "--json", default="Atlas.json")
    ap.add_argument("-p", "--padding", type=int, default=0)
    ap.add_argument("-e", "--extrude", type=int, default=0)
    ap.add_argument("--trim", action="store_true", help="裁剪透明边缘(默认关闭)")
    ap.add_argument("--pow2", action="store_true")
    ap.add_argument("--square", action="store_true")
    ap.add_argument("--max-size", type=int, default=8192)
    ap.add_argument("--dir", default=".")
    args = ap.parse_args()

    directory = os.path.abspath(args.dir)
    out_name = os.path.basename(args.output)
    json_name = os.path.basename(args.json)
    trim = args.trim

    ignore_patterns = load_ignore(directory)
    # 自己的输出也不要被打进图集
    exclude = {out_name, os.path.basename(args.output)}

    print(f"扫描目录: {directory}")
    if ignore_patterns:
        print(f".PackIgnore 规则: {ignore_patterns}")
    sprites = load_sprites(directory, exclude, ignore_patterns, trim)
    if not sprites:
        sys.exit("没有找到任何图像。")
    print(f"载入 {len(sprites)} 张图像 (裁剪透明边缘: {'开' if trim else '关'})")

    width, height, placements = search_size(
        sprites, args.padding, args.max_size, args.pow2, args.square)

    atlas = render_atlas(sprites, placements, width, height, args.extrude)
    out_png = os.path.join(directory, out_name)
    atlas.save(out_png)

    # 统计利用率
    used_area = sum(s["w"] * s["h"] for s in sprites)
    util = used_area / (width * height) * 100

    frames = {}
    for i, (x, y) in placements.items():
        s = sprites[i]
        frames[s["name"]] = {
            "frame": {"x": x, "y": y, "w": s["w"], "h": s["h"]},
            "rotated": False,
            "trimmed": (s["w"] != s["orig_w"] or s["h"] != s["orig_h"]),
            "spriteSourceSize": {
                "x": s["offset_x"], "y": s["offset_y"],
                "w": s["w"], "h": s["h"],
            },
            "sourceSize": {"w": s["orig_w"], "h": s["orig_h"]},
        }

    meta = {
        "image": out_name,
        "format": "RGBA8888",
        "size": {"w": width, "h": height},
        "scale": "1",
        "padding": args.padding,
        "extrude": args.extrude,
        "utilization": round(util, 2),
        "count": len(sprites),
    }

    out_json = os.path.join(directory, json_name)
    with open(out_json, "w", encoding="utf-8") as f:
        json.dump({"frames": frames, "meta": meta}, f, ensure_ascii=False, indent=2)

    print(f"图集: {out_png}  ({width}x{height})")
    print(f"JSON: {out_json}")
    print(f"空间利用率: {util:.2f}%")


if __name__ == "__main__":
    main()
