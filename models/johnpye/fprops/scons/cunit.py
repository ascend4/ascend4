import os
import pathlib
import platform
import shutil
import subprocess

from SCons.Script import AddMethod

_cunit_cache = {}


def _copy_cunit_result_to_env(env, result):
	env['WITH_CUNIT'] = result['ok']
	env['CUNIT_REASON'] = result['reason']
	if result['ok']:
		env['CUNIT_CPPPATH'] = list(result['cpppath'])
		env['CUNIT_LIBPATH'] = list(result['libpath'])
		env['CUNIT_LIBS'] = list(result['libs'])
	else:
		env['CUNIT_CPPPATH'] = []
		env['CUNIT_LIBPATH'] = []
		env['CUNIT_LIBS'] = []


def ensure_cunit(env):
	key = (
		env.subst('$CUNIT_PREFIX'),
		env.subst('$CUNIT_PKGPATH'),
		env['ENV'].get('PKG_CONFIG_PATH', ''),
		platform.system(),
	)
	result = _cunit_cache.get(key)
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
			cunit_pkgpath = env.subst('$CUNIT_PKGPATH')
			if cunit_pkgpath:
				probe_env.AppendENVPath('PKG_CONFIG_PATH', cunit_pkgpath)
			cunit_prefix = env.subst('$CUNIT_PREFIX')
			if cunit_prefix:
				for subdir in ('lib/pkgconfig', 'lib64/pkgconfig'):
					probe_env.AppendENVPath(
						'PKG_CONFIG_PATH',
						env.subst(f'$CUNIT_PREFIX/{subdir}')
					)
			probe_env['CPPPATH'] = None
			probe_env['LIBPATH'] = None
			probe_env['LIBS'] = None
			try:
				subprocess.run(
					[pkgtool, '--exists', 'cunit'],
					env=dict(os.environ, **probe_env['ENV']),
					stdout=subprocess.PIPE,
					stderr=subprocess.PIPE,
					check=True,
				)
			except Exception:
				result = {
					'ok': False,
					'reason': 'pkg-config --exists cunit failed',
					'cpppath': [],
					'libpath': [],
					'libs': [],
				}
			else:
				try:
					probe_env.ParseConfig(
						str(pathlib.Path(pkgtool)) + ' --cflags --libs cunit'
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
		_cunit_cache[key] = result

	_copy_cunit_result_to_env(env, result)
	return result['ok']


def generate(env):
	AddMethod(env.__class__, ensure_cunit, 'ensure_cunit')


def exists(env):
	return True
