import os
import pathlib
import platform
import shutil
import subprocess

from SCons.Script import AddMethod

_nlopt_cache = {}


def _copy_nlopt_result_to_env(env, result):
	env['HAVE_NLOPT'] = result['ok']
	env['NLOPT_REASON'] = result['reason']
	if result['ok']:
		env['NLOPT_CPPPATH'] = list(result['cpppath'])
		env['NLOPT_LIBPATH'] = list(result['libpath'])
		env['NLOPT_LIBS'] = list(result['libs'])
	else:
		env['NLOPT_CPPPATH'] = []
		env['NLOPT_LIBPATH'] = []
		env['NLOPT_LIBS'] = []


def ensure_nlopt(env):
	key = (
		env.subst('$NLOPT_PREFIX'),
		env['ENV'].get('PKG_CONFIG_PATH', ''),
		platform.system(),
	)
	result = _nlopt_cache.get(key)
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
			if probe_env.get('NLOPT_PREFIX') not in ('', '/usr', 'off'):
				for subdir in ('lib/pkgconfig', 'lib64/pkgconfig'):
					probe_env.AppendENVPath(
						'PKG_CONFIG_PATH',
						env.subst(f'$NLOPT_PREFIX/{subdir}')
					)
			probe_env['CPPPATH'] = None
			probe_env['LIBPATH'] = None
			probe_env['LIBS'] = None
			try:
				subprocess.run(
					[pkgtool, '--exists', 'nlopt'],
					env=dict(os.environ, **probe_env['ENV']),
					stdout=subprocess.PIPE,
					stderr=subprocess.PIPE,
					check=True,
				)
			except Exception:
				result = {
					'ok': False,
					'reason': 'pkg-config --exists nlopt failed',
					'cpppath': [],
					'libpath': [],
					'libs': [],
				}
			else:
				try:
					probe_env.ParseConfig(
						str(pathlib.Path(pkgtool)) + ' --cflags --libs nlopt'
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
		_nlopt_cache[key] = result

	_copy_nlopt_result_to_env(env, result)
	return result['ok']


def generate(env):
	AddMethod(env.__class__, ensure_nlopt, 'ensure_nlopt')


def exists(env):
	return True
