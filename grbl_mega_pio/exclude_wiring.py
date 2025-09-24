Import("env")

def skip_wiring_from_build(env, node):
    """
    Skip wiring.c from build process to avoid ISR conflicts
    `node.name` - a name of File System Node
    `node.get_path()` - a relative path  
    `node.get_abspath()` - an absolute path
    """
    # Check if this is the wiring.c file from Arduino framework
    if "wiring.c" in node.name:
        print(f"Excluding {node.get_path()} from build")
        # Return None to ignore this file from build process
        return None

    # Return the node unchanged for all other files
    return node

# Add the middleware to exclude wiring.c files
env.AddBuildMiddleware(skip_wiring_from_build, "*wiring.c")
