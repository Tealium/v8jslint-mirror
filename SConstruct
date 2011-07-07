import platform
import os

def IsLinux():
    p = platform.system() 
    return p.lower() == 'linux'

def IsWindows():
    p = platform.system() 
    return p.lower() == 'windows' or p.lower() == 'windows'

def IsDebug():
    o = GetOption('mode')
    return o and o.lower() == 'debug' 

# command line option to specify path to v8 source
AddOption('--v8',
           dest='v8',
           type='string',
           nargs=1,
           action='store',
           metavar='DIR',
           help='Path to v8 source')

# command line option to specify path to jslint source
AddOption('--jslint',
           dest='jslint',
           type='string',
           nargs=1,
           action='store',
           metavar='DIR',
           help='Path to jslint source')

# command line option to specify build mode
AddOption('--mode',
           dest='mode',
           type='string',
           nargs=1,
           action='store',
           help='Build configuration mode')

# verify v8 option has been specified
if not GetOption('v8'):
    print "Please specify the path to v8 : --v8=path"
    Exit(1)

# verify jslint option has been specified
if not GetOption('jslint'):
    print "Please specify the path to jslint : --jslint=path"
    Exit(1)

env = Environment(CPPPATH = ['.'])

if IsLinux():
    reswrap = 'reswrap --static --const -o $TARGET $SOURCE'

    if IsDebug():
        libs = ['v8_g', 'v8preparser_g', 'pthread']
        env.Append(CCFLAGS = ['-g', '-O0'])
        env.Append(CPPDEFINES = ['DEBUG'])
    else:
        libs = ['v8', 'v8preparser', 'pthread']

elif IsWindows():
    reswrap = 'reswrap --static         -o $TARGET $SOURCE'

    env.AppendENVPath('PATH', os.environ['Path'])
    env.AppendENVPath('INCLUDE', os.environ['Include'])
    env.AppendENVPath('LIB', os.environ['Lib'])

    libs = ['v8', 'v8preparser', 'Ws2_32', 'WINMM']

    if IsDebug():
        env.Append(CCFLAGS = ['/Od', '/Gm', '/Zi'])
        env.Append(CPPDEFINES = ['_DEBUG', 'DEBUG'])

# add builder to wrap javascript source into cc source
bld = Builder(action = reswrap)
env.Append(BUILDERS = {'Reswrapsource' : bld})

# wrap jslint.js into jslint.cc
env.Reswrapsource('jslint.cc', GetOption('jslint') + 'jslint.js')
# wrap lint.js into lint.cc
env.Reswrapsource('lint.cc', 'lint.js')

# dependencies on the javascript source
Depends('v8jslint.cc', 'jslint.cc')
Depends('jslint.cc', GetOption('jslint') + 'jslint.js')
Depends('lint.cc', 'lint.js')

env.Program('v8jslint', ['v8jslint.cc'], LIBS=libs, LIBPATH=GetOption('v8'))

# repositories
Repository(GetOption('v8') + 'include')
