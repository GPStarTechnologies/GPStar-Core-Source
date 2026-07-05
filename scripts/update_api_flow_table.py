#!/usr/bin/env python3
import re
from pathlib import Path

def get_source_root():
    """Get the source root by navigating from script location (up 1, then into source)."""
    script_path = Path(__file__).resolve()
    repo_root = script_path.parent.parent
    source_root = repo_root / 'source'
    return source_root

def extract_api_enums(source_root):
    """Extract and alphabetize all A_* enums from Communication.h."""
    comm_h = source_root / 'SharedLib' / 'Communication' / 'include' / 'Communication.h'
    text = comm_h.read_text()
    
    # Find all A_* identifiers in the file
    pattern = re.compile(r'\b(A_[A-Z0-9_]+)\b')
    apis = set(pattern.findall(text))
    
    # Filter out excluded enums
    excluded = {'A_COM_START', 'A_COM_END', 'A_CMD_NULL', 'A_CMD_NO_OP', 'A_DATA_NULL', 'A_DATA_NO_OP'}
    apis = apis - excluded
    
    # Alphabetize and return
    return sorted(apis)

def detect_device(folder_name):
    """Detect device from folder name."""
    s = folder_name.lower()
    if 'wand' in s or 'neutrona' in s:
        return 'W'
    if 'attenuator' in s:
        return 'A'
    if 'proton' in s or 'pack' in s:
        return 'P'
    return None

def find_api_calls(source_root):
    """Find all *SerialSend(), *SendData(), and executeCommand() calls in the source tree."""
    map_dir = {}
    
    # Pattern to match wrapper/command calls (includes *Send and *SendData variants)
    pattern = re.compile(r'(packSerialSend|packSerialSendData|attenuatorSerialSend|attenuatorSendData|wandSerialSend|wandSerialSendData|executeCommand)\s*\(\s*(A_[A-Z0-9_]+)')
    
    # Walk through all .h and .cpp files
    for file_path in source_root.rglob('*'):
        if file_path.suffix not in ('.h', '.cpp'):
            continue
        if 'TEMP_' in file_path.name:
            continue
        
        try:
            text = file_path.read_text(errors='ignore')
        except:
            continue
        
        # Determine source device from file path
        rel_path = file_path.relative_to(source_root)
        src = None
        for part in rel_path.parts:
            src = detect_device(part)
            if src:
                break
        
        if not src:
            continue
        
        # Find all matches in this file
        for match in pattern.finditer(text):
            wrapper, api = match.groups()
            
            # Determine destination from wrapper
            if wrapper in ('packSerialSend', 'packSerialSendData'):
                dst = 'P'
            elif wrapper in ('attenuatorSerialSend', 'attenuatorSendData'):
                dst = 'A'
            elif wrapper in ('wandSerialSend', 'wandSerialSendData'):
                dst = 'W'
            elif wrapper == 'executeCommand':
                # Skip executeCommand for now (generic, context-dependent)
                continue
            else:
                continue
            
            pair = f"{src}->{dst}"
            # Only record the four canonical directions
            if pair in ('P->A', 'A->P', 'P->W', 'W->P'):
                key = api.strip()
                if key not in map_dir:
                    map_dir[key] = set()
                map_dir[key].add(pair)
    
    return map_dir

def update_api_flow_table(source_root, api_list, map_dir):
    """Update the API_FLOW.md table with current findings."""
    api_flow_path = source_root / 'SharedLib' / 'Communication' / 'API_FLOW.md'
    
    text = api_flow_path.read_text()
    lines = text.splitlines()
    
    # Find table start and end
    start = None
    for i, l in enumerate(lines):
        if l.startswith('| API Name'):
            start = i
            break
    
    if start is None:
        raise SystemExit('Table start not found')
    
    # Find end: first '---' separator after header
    end = None
    for j in range(start + 1, len(lines)):
        if lines[j].startswith('---'):
            end = j
            break
    
    if end is None:
        raise SystemExit('Table end marker (---) not found')
    
    # Build new table
    # Calculate max API name length + 2 for padding (1 space before/after)
    max_api_len = max(len(api) for api in api_list) + 2
    
    # Build dynamic header and separator
    api_name_header = 'API Name'
    header = f'| {api_name_header:<{max_api_len}} | P --> A | A --> P | P --> W | W --> P |'
    sep_dashes = '-' * (max_api_len - 2)
    sep = f'| {sep_dashes:<{max_api_len}} | ------- | ------- | ------- | ------- |'
    new_table = [header, sep]
    
    # Add rows for each API in alphabetical order
    for api in api_list:
        p_a = 'X' if api in map_dir and 'P->A' in map_dir[api] else ''
        a_p = 'X' if api in map_dir and 'A->P' in map_dir[api] else ''
        p_w = 'X' if api in map_dir and 'P->W' in map_dir[api] else ''
        w_p = 'X' if api in map_dir and 'W->P' in map_dir[api] else ''
        new_line = f'| {api:<{max_api_len}} | {p_a:^7} | {a_p:^7} | {p_w:^7} | {w_p:^7} |'
        new_table.append(new_line)
    
    # Find unmatched APIs (in api_list but not in map_dir)
    unmatched = sorted([api for api in api_list if api not in map_dir])
    
    # Build unmatched section
    unmatched_section = []
    if unmatched:
        unmatched_section.append('')
        unmatched_section.append('## Unmatched API Names')
        unmatched_section.append('')
        for api in unmatched:
            unmatched_section.append(f'- {api}')
    
    # Assemble new file content: prefix + new_table + unmatched_section + suffix
    new_lines = lines[:start] + new_table + unmatched_section + lines[end:]
    new_content = '\n'.join(new_lines) + '\n'
    
    # Write back to file
    api_flow_path.write_text(new_content)
    print(f'Updated {api_flow_path}')

def main():
    source_root = get_source_root()
    print(f'Source root: {source_root}')
    
    # Step 2-3: Extract and alphabetize APIs from Communication.h
    print('Extracting A_* enums from Communication.h...')
    api_list = extract_api_enums(source_root)
    print(f'Found {len(api_list)} APIs')
    
    # Step 4: Find all serial send and execute command calls in source tree
    print('Searching for API calls in source tree...')
    map_dir = find_api_calls(source_root)
    print(f'Found mappings for {len(map_dir)} APIs')
    
    # Step 5-6: Update API_FLOW.md with new table
    print('Updating API_FLOW.md table...')
    unmatched = [api for api in api_list if api not in map_dir]
    update_api_flow_table(source_root, api_list, map_dir)
    print(f'Done! ({len(unmatched)} unmapped APIs listed after --- separator)')

if __name__ == '__main__':
    main()
