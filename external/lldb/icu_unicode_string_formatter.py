import lldb

def unicode_string_summary(valobj, internal_dict):
    # Simple approach - try to directly extract the data
    try:
        # Get flags and determine if it's using stack buffer
        flags = valobj.GetChildMemberWithName('fUnion').GetChildMemberWithName('fStackFields').GetChildMemberWithName('fLengthAndFlags').GetValueAsUnsigned()
        using_stack = bool(flags & 2)  # kUsingStackBuffer = 2
        
        # Get the length
        if flags >= 0:
            length = flags >> 5  # kLengthShift = 5
        else:
            length = valobj.GetChildMemberWithName('fUnion').GetChildMemberWithName('fFields').GetChildMemberWithName('fLength').GetValueAsSigned()
        
        # Access raw memory directly using LLDB's memory reading capabilities
        if using_stack:
            # For stack buffer
            buffer_addr = valobj.GetChildMemberWithName('fUnion').GetChildMemberWithName('fStackFields').GetChildMemberWithName('fBuffer').GetLoadAddress()
        else:
            # For heap buffer
            buffer_addr = valobj.GetChildMemberWithName('fUnion').GetChildMemberWithName('fFields').GetChildMemberWithName('fArray').GetValueAsUnsigned()
        
        if buffer_addr == 0:
            return f'(length: {length}) "<null>"'
        
        # Read memory directly - 2 bytes per char16_t
        error = lldb.SBError()
        data = valobj.GetProcess().ReadMemory(buffer_addr, min(length * 2, 100), error)
        
        if error.Success():
            # Convert raw bytes to UTF-16 string
            string_content = ""
            for i in range(0, len(data), 2):
                if i+1 < len(data):
                    char_code = (data[i+1] << 8) | data[i]  # Little-endian
                    string_content += chr(char_code)
            
            if length > 16:
                string_content = string_content[:16] + "..."
            
            return f'"{string_content}" (length: {length})'
    
    except Exception as e:
        pass
    
    # Fallback - return just the length
    try:
        flags = valobj.GetChildMemberWithName('fUnion').GetChildMemberWithName('fStackFields').GetChildMemberWithName('fLengthAndFlags').GetValueAsUnsigned()
        if flags >= 0:
            length = flags >> 5
        else:
            length = valobj.GetChildMemberWithName('fUnion').GetChildMemberWithName('fFields').GetChildMemberWithName('fLength').GetValueAsSigned()
        
        return f'(length: {length}) "<content extraction failed>"'
    except:
        return "(error retrieving string info)"

def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand('type summary add -F icu_unicode_string_formatter.unicode_string_summary "nugdev::compiler::lib::String"')
    
    print("ICU UnicodeString formatter is installed.")