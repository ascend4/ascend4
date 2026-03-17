# a copy of the file in ~/ascend/scons/ipopt.py -- to allow self-contained FPROPS builds
import pathlib
import platform
import shutil
import subprocess

from SCons.Script import AddMethod

_ipopt_cache = {}

def _copy_ipopt_result_to_env(env, result):
	env['HAVE_IPOPT'] = result['ok']
	env['IPOPT_REASON'] = result['reason']
	if result['ok']:
		env['IPOPT_CPPPATH'] = list(result['cpppath'])
		env['IPOPT_LIBPATH'] = list(result['libpath'])
		env['IPOPT_LIBS'] = list(result['libs'])
	else:
		env['IPOPT_CPPPATH'] = []
		env['IPOPT_LIBPATH'] = []
		env['IPOPT_LIBS'] = []

def ensure_ipopt(env):
	key = (
		env.subst('$IPOPT_PREFIX'),
		env['ENV'].get('PKG_CONFIG_PATH', ''),
		platform.system(),
	)
	result = _ipopt_cache.get(key)
	if result is None:
		pkgtool = shutil.which('pkg-config') or shutil.which('pkgconf')
		if pkgtool is None:
			result = {
				'ok': False,
				'reason': 'pkg-config not found',
				'cpppath': [],
				'libpath': [],
				'libs': [],
			}
		else:
			probe_env = env.Clone()
			if probe_env.get('IPOPT_PREFIX') not in ['/usr']:
				for subdir in ('lib/pkgconfig', 'lib64/pkgconfig'):
					probe_env.AppendENVPath(
						'PKG_CONFIG_PATH',
						env.subst(f'$IPOPT_PREFIX/{subdir}')
					)
			probe_env['CPPPATH'] = None
			probe_env['LIBPATH'] = None
			probe_env['LIBS'] = None
			try:
				subprocess.run(
					[pkgtool, '--exists', 'ipopt'],
					stdout=subprocess.PIPE,
					stderr=subprocess.PIPE,
					check=True
				)
			except Exception:
				result = {
					'ok': False,
					'reason': 'pkg-config --exists ipopt failed',
					'cpppath': [],
					'libpath': [],
					'libs': [],
				}
			else:
				try:
					probe_env.ParseConfig(
						str(pathlib.Path(pkgtool)) + ' --cflags --libs ipopt'
					)
					result = {
						'ok': True,
						'reason': None,
						'cpppath': list(probe_env.get('CPPPATH') or []),
						'libpath': list(probe_env.get('LIBPATH') or []),
						'libs': list(probe_env.get('LIBS') or []),
					}
				except Exception:
					result = {
						'ok': False,
						'reason': 'pkg-config parse failed',
						'cpppath': [],
						'libpath': [],
						'libs': [],
					}
		_ipopt_cache[key] = result

	_copy_ipopt_result_to_env(env, result)
	return result['ok']

def generate(env):
	AddMethod(env.__class__, ensure_ipopt, 'ensure_ipopt')

def exists(env):
	return True
