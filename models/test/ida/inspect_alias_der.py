#!/usr/bin/env python3

import sys

import ascpy


def dump(obj, label):
    print("LABEL", label)
    print("TYPE", type(obj))
    print("REPR", repr(obj))
    for name in dir(obj):
        if "child" in name.lower() or "instance" in name.lower() or "name" in name.lower() or "type" in name.lower():
            print(" ", name)


def main():
    model = sys.argv[1]
    lib = ascpy.Library()
    lib.load("models/test/ida/alias_der_wLINK.a4c")
    typ = lib.findType(model)
    sim = typ.getSimulation("s", False)
    dump(sim, "sim")
    children = sim.getChildren()
    print("TOP_CHILDREN_TYPE", type(children))
    print("TOP_CHILDREN", children)
    for child in children:
        print("TOP_CHILD_NAME", child.getName(), type(child.getName()))
    root = children[0]
    dump(root, "root")
    for path in ("s", "s.cell", "s.cell.y", "s.cell.y[1]", "s.y", "s.y[1]", "s.cell.dy_dt[1]", "s.dy_dt[1]"):
        try:
            got = sim.checkInstance(path)
            print("CHECK", path, "->", got, type(got))
        except Exception as err:
            print("CHECK_ERR", path, err)
    root_children = root.getChildren()
    print("ROOT_CHILDREN", root_children)
    for child in root_children:
        print("ROOT_CHILD_NAME", child.getName(), type(child.getName()))
    top = {str(child.getName()): child for child in root_children}
    print("ROOT_NAMES", sorted(top.keys()))
    for key in ("cell", "y", "dy_dt"):
        obj = top[key]
        dump(obj, key)
        if hasattr(obj, "getChildren"):
            sub = obj.getChildren()
            print("CHILDREN", key, sub)
            for child in sub:
                print("SUB_CHILD_NAME", key, child.getName(), type(child.getName()))
            subd = {str(child.getName()): child for child in sub}
            if "1" in subd:
                dump(subd["1"], f"{key}[1]")
                if key == "cell":
                    cell_children = subd
    cell_y = {str(child.getName()): child for child in top["cell"].getChildren()}["y"]
    cell_dy = {str(child.getName()): child for child in top["cell"].getChildren()}["dy_dt"]
    cell_y1 = {str(child.getName()): child for child in cell_y.getChildren()}["1"]
    top_y1 = {str(child.getName()): child for child in top["y"].getChildren()}["1"]
    cell_dy1 = {str(child.getName()): child for child in cell_dy.getChildren()}["1"]
    top_dy1 = {str(child.getName()): child for child in top["dy_dt"].getChildren()}["1"]
    print("COMPARE y1 eq", cell_y1 == top_y1)
    print("COMPARE dy1 eq", cell_dy1 == top_dy1)
    print("CELL_Y1_REPR", repr(cell_y1))
    print("TOP_Y1_REPR", repr(top_y1))
    print("CELL_DY1_REPR", repr(cell_dy1))
    print("TOP_DY1_REPR", repr(top_dy1))


if __name__ == "__main__":
    main()
