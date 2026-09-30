# Actually compiles every .fs in this folder on the real GPU, using the same
# preamble ISFBrowser.cpp builds (INPUTS -> uniform declarations), so GLSL
# errors (reserved words, undeclared identifiers, etc.) surface here instead
# of only inside Resolume. Needs `pip install moderngl` and a working GL
# context; run with plain `python3 check_shaders.py`.
import re, sys, glob
import moderngl

def check(path):
    src = open(path).read()
    end = src.find('*/')
    if end < 0:
        return None, "no /*{ ... }*/ ISF header found"
    header = src[:end]
    body = src[end+2:]
    if body.startswith('\n'):
        body = body[1:]

    names = re.findall(r'"NAME"\s*:\s*"([^"]+)"', header)
    preamble_uniforms = "\n".join(f"uniform float {n};" for n in names)

    fs_src = f"""#version 330 core
uniform float TIME;
uniform float TIMEDELTA;
uniform int   FRAMEINDEX;
uniform vec2  RENDERSIZE;
uniform float uInstanceSeed;
in vec2 isf_FragNormCoord;
out vec4 fragColor;
{preamble_uniforms}
#define gl_FragColor fragColor
{body}
"""
    vs_src = """#version 330 core
in vec2 in_vert;
out vec2 isf_FragNormCoord;
void main() {
    gl_Position = vec4(in_vert, 0.0, 1.0);
    isf_FragNormCoord = in_vert * 0.5 + 0.5;
}
"""
    try:
        ctx.program(vertex_shader=vs_src, fragment_shader=fs_src)
        return len(names), None
    except Exception as e:
        return len(names), str(e)

ctx = moderngl.create_standalone_context()
ok = True
for path in sorted(glob.glob('/home/ivo/git/freeframe1_resolume/031-isf-browser/*.fs')):
    n, err = check(path)
    name = path.split('/')[-1]
    if err:
        ok = False
        print(f"FAIL  {name}  ({n} inputs)")
        print("  " + err.replace("\n", "\n  "))
    else:
        print(f"OK    {name}  ({n} inputs)")
sys.exit(0 if ok else 1)
