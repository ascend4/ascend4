# a copy of the file in ~/ascend/scons/ipopt.py -- to allow self-contained FPROPS builds
import os
import pathlib
import platform
import shlex
import shutil
import subprocess

from SCons.Script import AddMethod

_ipopt_cache = {}

def _unique_paths(values):
	seen = set()
	result = []
	for value in values or []:
		if not value:
			continue
		key = os.path.normcase(os.path.normpath(str(value)))
		if key in seen:
			continue
		seen.add(key)
		result.append(str(value))
	return result

def _default_hsl_names():
	if platform.system() == 'Windows':
		return ['coinhsl.dll', 'libcoinhsl.dll', 'hsl.dll', 'libhsl.dll']
	if platform.system() == 'Darwin':
		return ['libcoinhsl.dylib', 'libhsl.dylib']
	return ['libcoinhsl.so', 'libhsl.so']

def _candidate_hsl_pkgconfig_dirs(env):
	dirs = []
	prefix = env.subst('$HSL_PREFIX')
	if prefix and prefix not in ('', 'off'):
		prefix_path = pathlib.Path(prefix)
		dirs.extend([
			prefix_path / 'lib/pkgconfig',
			prefix_path / 'lib64/pkgconfig',
		])
		dirs.extend(prefix_path.glob('lib/*-linux-gnu/pkgconfig'))

	home = env['ENV'].get('HOME') or os.environ.get('HOME')
	if home:
		home_local = pathlib.Path(home) / '.local'
		dirs.extend([
			home_local / 'lib/pkgconfig',
			home_local / 'lib64/pkgconfig',
		])
		dirs.extend(home_local.glob('lib/*-linux-gnu/pkgconfig'))

	return _unique_paths(
		str(path)
		for path in dirs
		if path and pathlib.Path(path).exists()
	)

def _candidate_hsl_dirs(env, ipopt_result):
	dirs = []
	for path in ipopt_result.get('libpath') or []:
		dirs.append(path)

	prefix = env.subst('$IPOPT_PREFIX')
	if prefix and prefix not in ('', 'off'):
		prefix_path = pathlib.Path(prefix)
		dirs.extend([
			prefix_path / 'lib',
			prefix_path / 'lib64',
		])
		dirs.extend(prefix_path.glob('lib/*-linux-gnu'))

	home = env['ENV'].get('HOME') or os.environ.get('HOME')
	if home:
		home_local = pathlib.Path(home) / '.local'
		dirs.extend([
			home_local / 'lib',
			home_local / 'lib64',
		])
		dirs.extend(home_local.glob('lib/*-linux-gnu'))

	return _unique_paths(
		str(path)
		for path in dirs
		if path and pathlib.Path(path).exists()
	)

def _parse_pkgconfig_lib_output(text):
	libdirs = []
	libs = []
	for token in shlex.split(text or ''):
		if token.startswith('-L'):
			libdirs.append(token[2:])
		elif token.startswith('-l'):
			libs.append(token[2:])
	return _unique_paths(libdirs), _unique_paths(libs)

def _find_library_in_dirs(libdirs, libs):
	suffixes = {
		'Windows': '.dll',
		'Darwin': '.dylib',
	}.get(platform.system(), '.so')
	for libdir in libdirs:
		base = pathlib.Path(libdir)
		for lib in libs:
			candidate = base / ('lib' + lib + suffixes)
			if candidate.exists():
				return str(candidate)
			candidate = base / (lib + suffixes)
			if candidate.exists():
				return str(candidate)
	return None

def _find_hsl_from_pkgconfig(env, pkgtool):
	probe_env = env.Clone()
	for subdir in _candidate_hsl_pkgconfig_dirs(env):
		probe_env.AppendENVPath('PKG_CONFIG_PATH', subdir)
	try:
		subprocess.run(
			[pkgtool, '--exists', 'coinhsl'],
			env=dict(os.environ, **probe_env['ENV']),
			stdout=subprocess.PIPE,
			stderr=subprocess.PIPE,
			check=True
		)
	except Exception:
		return None

	try:
		lib_output = subprocess.run(
			[pkgtool, '--libs', 'coinhsl'],
			env=dict(os.environ, **probe_env['ENV']),
			stdout=subprocess.PIPE,
			stderr=subprocess.PIPE,
			check=True,
			text=True
		).stdout
	except Exception:
		return {
			'hsl_probe_target': None,
			'hsl_found': False,
			'hsl_library': None,
			'hsl_reason': 'pkg-config --libs coinhsl failed',
		}

	libdirs, libs = _parse_pkgconfig_lib_output(lib_output)
	library = _find_library_in_dirs(libdirs, libs)
	if library:
		return {
			'hsl_probe_target': library,
			'hsl_found': True,
			'hsl_library': library,
			'hsl_reason': None,
		}

	return {
		'hsl_probe_target': None,
		'hsl_found': False,
		'hsl_library': None,
		'hsl_reason': 'coinhsl.pc found but shared library was not resolved',
	}

def _find_hsl_probe_target(env, ipopt_result):
	pkgtool = shutil.which('pkg-config') or shutil.which('pkgconf')
	if pkgtool is not None:
		pkg_result = _find_hsl_from_pkgconfig(env, pkgtool)
		if pkg_result and pkg_result.get('hsl_found'):
			return pkg_result

	for directory in _candidate_hsl_dirs(env, ipopt_result):
		for name in _default_hsl_names():
			candidate = pathlib.Path(directory) / name
			if candidate.exists():
				return {
					'hsl_probe_target': str(candidate),
					'hsl_found': True,
					'hsl_library': str(candidate),
					'hsl_reason': None,
				}

	return {
		'hsl_probe_target': _default_hsl_names()[0],
		'hsl_found': False,
		'hsl_library': None,
		'hsl_reason': 'libcoinhsl not found in expected locations',
	}

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
	env['IPOPT_HSL_FOUND'] = bool(result.get('hsl_found'))
	env['IPOPT_HSL_LIBRARY'] = result.get('hsl_library')
	env['IPOPT_HSL_PROBE_TARGET'] = result.get('hsl_probe_target')
	env['IPOPT_HSL_REASON'] = result.get('hsl_reason')
	env['IPOPT_HSL_LOADABLE'] = False
	env['IPOPT_HSL_MA27_AVAILABLE'] = False
	env['IPOPT_HSL_MA97_AVAILABLE'] = False
	env['IPOPT_HSL_STATUS'] = result.get('hsl_reason') or 'not checked'

def ensure_ipopt(env):
	key = (
		env.subst('$IPOPT_PREFIX'),
		env.subst('$HSL_PREFIX'),
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
		if result['ok']:
			result.update(_find_hsl_probe_target(env, result))
		else:
			result.update({
				'hsl_probe_target': None,
				'hsl_found': False,
				'hsl_library': None,
				'hsl_reason': result['reason'],
			})
		_ipopt_cache[key] = result

	_copy_ipopt_result_to_env(env, result)
	return result['ok']

def generate(env):
	AddMethod(env.__class__, ensure_ipopt, 'ensure_ipopt')

def exists(env):
	return True
