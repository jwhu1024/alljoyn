AJ_CLI_SRC = Glob('aj_client.c')
AJ_SRV_SRC = Glob('aj_service.c')

VARIANT = ARGUMENTS.get('VARIANT', 'debug')
vars = Variables(None,ARGUMENTS)
vars.Add('debug','Set to 0 to build for debug', 'debug')
env = Environment(variables = vars)
Help(vars.GenerateHelpText(env))

if VARIANT == 'debug':
	env.Append(CCFLAGS = ['-g', '-DQCC_OS_GROUP_POSIX'])
else:
	env.Append(CCFLAGS = ['-O2', '-DQCC_OS_GROUP_POSIX'])

env.Append(LIBS = ['alljoyn_c', 'alljoyn'])
env.Append(LIBPATH = ['./lib/'])
env.Append(CPPPATH = ['./inc/'])
	
env.Program(source = AJ_CLI_SRC, target = 'aj_c_client')
env.Program(source = AJ_SRV_SRC, target = 'aj_c_service')

