# Setting source for alljoyn client
AJ_CLI_SRC = Glob('aj_client.c')

# Setting source for alljoyn service
AJ_SRV_SRC = Glob('aj_service.c')

# Get argument from command-line
VARIANT = ARGUMENTS.get('VARIANT', 'debug')
vars = Variables(None,ARGUMENTS)
vars.Add('debug','Set to 0 to build for debug', 'debug')
env = Environment(variables = vars)
Help(vars.GenerateHelpText(env))

# debug version will compile with -g flag to enable debug symbol table
if VARIANT == 'debug':
	env.Append(CCFLAGS = ['-g', '-DDEBUG', '-DQCC_OS_GROUP_POSIX'])
else:
	env.Append(CCFLAGS = ['-O2', '-DQCC_OS_GROUP_POSIX'])

# Set compiler flag before building
env.Append(LIBS = ['alljoyn_c', 'alljoyn'])	# library to be linked
env.Append(LIBPATH = ['./lib/'])			# library path
env.Append(CPPPATH = ['./inc/'])			# header files path

# start to compile
env.Program(source = AJ_CLI_SRC, target = 'aj_c_client')
env.Program(source = AJ_SRV_SRC, target = 'aj_c_service')

