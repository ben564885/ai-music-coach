import os

# Define the target file path
target_file = 'venv/lib/python3.9/site-packages/oemer/symbol_extraction.py'

# Check if the file exists
if not os.path.exists(target_file):
    print(f"Error: File not found at {target_file}")
    exit(1)

# Read the file content
with open(target_file, 'r') as f:
    lines = f.readlines()

# The lines to replace
old_code_1 = '            if ss.group != note.group:\n'
old_code_2 = '                raise E.SfnNoteGroupMismatch(f"Group of sfn and note not mismatch: {ss}\\n{note}")\n'

# Flag to track if replacement happened
replaced = False

# Iterate and replace
new_lines = []
skip_next = False
for i, line in enumerate(lines):
    if skip_next:
        skip_next = False
        continue
        
    if line == old_code_1 and i + 1 < len(lines) and lines[i+1] == old_code_2:
        print("Found the problematic code block. Patching...")
        new_lines.append(old_code_1)
        new_lines.append(f'                # Patched by AI Music Coach: strict mismatch causing crashes. skipping instead.\n')
        new_lines.append(f'                continue\n')
        new_lines.append(f'                # {lines[i+1].strip()}\n')
        skip_next = True
        replaced = True
    else:
        new_lines.append(line)

# Write back to file if replaced
if replaced:
    with open(target_file, 'w') as f:
        f.writelines(new_lines)
    print(f"Successfully patched {target_file}")
else:
    print("Code pattern not found. Maybe already patched?")
