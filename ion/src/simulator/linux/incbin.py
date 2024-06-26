# This script generates:
# - .s file representing assets in order to access them from C code
# - .h representing the mapping between symbols and assets

import sys
import re
import argparse
import io
import os

parser = argparse.ArgumentParser(description="Process some asset files.")
parser.add_argument(
    "assets", metavar="asset", type=str, nargs="+", help="The list of assets to include"
)
parser.add_argument("-o", help="The file to generate")
args = parser.parse_args()


def asset_basename_symbol(asset):
    return os.path.splitext(asset)[0].replace("-", "_")


def print_asset(f, asset):
    asset_basename = asset_basename_symbol(asset)
    f.write(".global _ion_simulator_" + asset_basename + "_start\n")
    f.write(".global _ion_simulator_" + asset_basename + "_end\n")
    f.write("_ion_simulator_" + asset_basename + "_start:\n")
    f.write('    .incbin "ion/src/simulator/assets/' + asset + '"\n')
    f.write("_ion_simulator_" + asset_basename + "_end:\n\n")


def print_assembly(files, path):
    f = open(path, "w")
    for asset in files:
        print_asset(f, asset)
    f.close()


def print_declaration(f, asset):
    asset_basename = asset_basename_symbol(asset)
    f.write("extern unsigned char _ion_simulator_" + asset_basename + "_start;\n")
    f.write("extern unsigned char _ion_simulator_" + asset_basename + "_end;\n")


def print_mapping(f, asset):
    asset_basename = asset_basename_symbol(asset)
    f.write(
        'ResourceMap("'
        + asset
        + '", &_ion_simulator_'
        + asset_basename
        + "_start, &_ion_simulator_"
        + asset_basename
        + "_end),\n"
    )


def print_header(files, path):
    f = open(path, "w")
    f.write("#ifndef ION_SIMULATOR_LINUX_IMAGES_H\n")
    f.write("#define ION_SIMULATOR_LINUX_IMAGES_H\n\n")
    f.write("// This file is auto-generated by incbin.py\n\n")

    for asset in files:
        print_declaration(f, asset)

    f.write("\nclass ResourceMap {\n")
    f.write("public:\n")
    f.write(
        "  constexpr ResourceMap(const char * identifier, unsigned char * start, unsigned char * end) : m_identifier(identifier), m_start(start), m_end(end) {}\n"
    )
    f.write("  const char * identifier() const { return m_identifier; }\n")
    f.write("  unsigned char * start() const { return m_start; }\n")
    f.write("  unsigned char * end() const { return m_end; }\n")
    f.write("private:\n")
    f.write("  const char * m_identifier;\n")
    f.write("  unsigned char * m_start;\n")
    f.write("  unsigned char * m_end;\n")
    f.write("};\n\n")
    f.write("constexpr static ResourceMap resources_addresses[] = {\n")
    for asset in files:
        print_mapping(f, asset)

    f.write("};\n\n")
    f.write("#endif\n")
    f.close()


if args.o.endswith(".s"):
    print_assembly(args.assets, args.o)
if args.o.endswith(".h"):
    print_header(args.assets, args.o)
