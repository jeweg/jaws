#!/usr/bin/env python3

import sys
import os
import re

# ================================================================================
#
# Callbacks to create the actual output.
#

INCLUDE_FILE_PATH = '../../src/vulkan/to_string.hpp'
IMPL_FILE_PATH = '../../src/vulkan/to_string.cpp'

include_file_code = ''
impl_file_code = ''

def begin_enum(enum_name):
    global include_file_code, impl_file_code
    include_file_code += f'extern JAWS_API absl::string_view to_string({enum_name}) noexcept;\n'
    impl_file_code += f'''\
absl::string_view to_string({enum_name} v) noexcept
{{
    switch (v) {{
    default: return "unknown_value_for_{enum_name}";
'''


def end_enum(enum_name):
    global impl_file_code
    impl_file_code += f'''\
    }}
}}

'''


def enum_value_case(enum_value_name, numeric_enum_value, comment):
    global impl_file_code
    impl_file_code += f'''\
    case {numeric_enum_value}: return "{enum_value_name}"; {f'// {comment}' if comment else ''}
'''


def begin_extension_specific_block(enum_name, extension_name):
    global impl_file_code
    impl_file_code += f'#if defined({extension_name})\n'


def end_extension_specific_block(enum_name, extension_name):
    global impl_file_code
    impl_file_code += f'#endif\n'


def finish():
    this_script_path = os.path.dirname(os.path.realpath(__file__))
    include_file_path = os.path.join(this_script_path, INCLUDE_FILE_PATH)
    impl_file_path = os.path.join(this_script_path, IMPL_FILE_PATH)

    def insert_into_file(filepath, inserted_text):
        re_begin = re.compile('\s*//\s*#BEGIN#.*')
        re_end = re.compile('\s*//\s*#END#.*')

        out_lines = []
        before_begin = True
        before_end = True
        with open(filepath, 'r') as file:
            for line in file:
                if before_begin:
                    if re_begin.match(line):
                        out_lines.append(line)
                        out_lines.append(inserted_text)
                        before_begin = False
                    else:
                        out_lines.append(line)
                if not before_begin and before_end:
                    if re_end.match(line):
                        before_end = False
                if not before_begin and not before_end:
                    out_lines.append(line)

        with open(filepath, 'w') as file:
            file.write(''.join(out_lines))
        print(f'Processed {filepath}.')

    insert_into_file(include_file_path, include_file_code)
    insert_into_file(impl_file_path, impl_file_code)


# ================================================================================
#
# Parse the Vulkan spec and call the callbacks above.
#

import urllib.request
from xml.etree import ElementTree

if __name__ != "__main__":
    print("This file is meant to be run as a script, exiting.")
    sys.exit(0)

VK_SPEC_PATH = "https://raw.githubusercontent.com/KhronosGroup/Vulkan-Docs/master/xml/vk.xml"
 
if VK_SPEC_PATH.startswith("http"):
    file = urllib.request.urlopen(VK_SPEC_PATH)
else:
    file = open(VK_SPEC_PATH, 'r')
with file:
    spec = ElementTree.parse(file)

# Build a data structure to hold all enum extensions:
# dict(enum-name -> dict( ext-name -> [(enum-value-name, numeric-enum-value), ...] ) )
enum_extension_map = {}

for extension in spec.findall('extensions/extension'):
    ext_name = extension.get('name')

    if  extension.get('provisional'):
        print(f'Ignoring provisional extension {ext_name}')
        continue

    ext_number = extension.get('number')
    for enum_value in extension.findall('require/enum'):
        extended_enum = enum_value.get('extends')
        if (not extended_enum):
            # non-"extents" enum values are just global constants.
            continue

        if (enum_value.get('alias')):
            # no distinct value, instead this aliased another enum value of the same enum.
            # We ignore these at the moment.
            continue

        if (enum_value.get('bitpos')):
            # Not an enum value in our sense, but a bit in a mask.
            continue

        offset = enum_value.get('offset')
        if (not offset):
            # There is at least one legacy special case that still has no offset attribute
            # even when excluding all other cases above.
            # The special case is the extension to VkSamplerAddressMode in VK_KHR_sampler_mirror_clamp_to_edge.
            continue

        # extnumber may be overridden by an enum.
        used_ext_number = ext_number
        if (enum_value.get('extnumber')):
            used_ext_number = int(enum_value.get('extnumber'))
        else:
            used_ext_number = int(ext_number)

        # Compute the actual enumerant value as specified at
        # https://www.khronos.org/registry/vulkan/specs/1.2/styleguide.html#_assigning_extension_token_values
        numeric_value = 1000000000 + (used_ext_number - 1) * 1000 + int(offset)
        if enum_value.get('dir') == '-':
            numeric_value = -numeric_value
        
        if (not extended_enum in enum_extension_map):
            enum_extension_map[extended_enum] = {}

        if (not ext_name in enum_extension_map[extended_enum]):
            enum_extension_map[extended_enum][ext_name] = []

        enum_extension_map[extended_enum][ext_name].append((enum_value.get('name'), numeric_value))


for enum in spec.findall('./enums'):
    if (enum.get('type') != 'enum'):
        continue
    continue

    enum_name = enum.get('name')
    begin_enum(enum_name)

    for enum_value in enum.findall('enum'):
        enum_value_case(enum_value.get('name'), enum_value.get('value'), enum_value.get('comment'))

    # Add any extensions to this enum
    if enum_name in enum_extension_map:
        for ext_name, extensions_to_this_enum in enum_extension_map[enum_name].items():
            begin_extension_specific_block(enum_name, ext_name)
            for enum_value_name, numeric_enum_value in extensions_to_this_enum:
                enum_value_case(enum_value_name, numeric_enum_value, None)
            end_extension_specific_block(enum_name, ext_name)

    end_enum(enum_name)

finish()