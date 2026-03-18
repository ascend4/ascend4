# -*- coding: utf8 -*-
import sys

import extpy


_PLOT_READY = False
_PLOT_ERROR = None


def _report_note(msg):
	try:
		browser = extpy.getbrowser()
		if browser:
			browser.reporter.reportNote(msg)
			return
	except Exception:
		pass
	sys.stderr.write(msg + "\n")


def _report_error(msg):
	try:
		browser = extpy.getbrowser()
		if browser:
			browser.reporter.reportError(msg)
			return
	except Exception:
		pass
	sys.stderr.write(msg + "\n")


def _ensure_plotting():
	global _PLOT_READY, _PLOT_ERROR
	if _PLOT_READY:
		return True
	try:
		import loading
		loading.load_matplotlib(throw=True)
		import matplotlib.pyplot as plt
		globals()["plt"] = plt
		_PLOT_READY = True
		_PLOT_ERROR = None
		return True
	except Exception as e:
		_PLOT_ERROR = e
		_report_error(
			"Plotting is unavailable: unable to load GTK/matplotlib (%s)." % str(e)
		)
		return False


def _unwrap_model(self):
	"""
	Accept either the freeboard model itself or the demo wrapper that
	contains `fb` as a child model.
	"""
	try:
		return self.fb
	except Exception:
		return self


def _get_profile_arrays(model):
	n = model.n_segs.getIntValue()
	z_mm = []
	y_h2 = []
	y_h2o = []
	y_ar = []
	for i in range(n + 1):
		z_mm.append(float(model.z[i].to("mm")))
		y_h2.append(float(model.y_H2[i]))
		y_h2o.append(float(model.y_H2O[i]))
		y_ar.append(float(model.y_Ar[i]))
	return z_mm, y_h2, y_h2o, y_ar


def _profile_meta(model):
	try:
		t_k = float(model.T.to("K"))
	except Exception:
		t_k = float(model.T)
	try:
		p_bar = float(model.p.to("bar"))
	except Exception:
		p_bar = float(model.p) / 1e5
	try:
		l_mm = float(model.L.to("mm"))
	except Exception:
		l_mm = float(model.L) * 1e3
	return t_k, p_bar, l_mm


def freeboard_plot_profile(self):
	"""Plot H2, H2O and Ar mole fractions versus freeboard depth."""
	if not _ensure_plotting():
		return

	model = _unwrap_model(self)
	z_mm, y_h2, y_h2o, y_ar = _get_profile_arrays(model)
	t_k, p_bar, l_mm = _profile_meta(model)

	series = [
		("H2", y_h2, "tab:red", "o-"),
		("H2O", y_h2o, "tab:blue", "s-"),
		("Ar", y_ar, "tab:green", "^-"),
	]

	max_level = max(max(vals) for _, vals, _, _ in series)
	zoom_cutoff = 0.05 * max_level
	zoom_species = [
		(name, vals, color, style)
		for name, vals, color, style in series
		if max(vals) <= zoom_cutoff
	]

	plt.ioff()
	if zoom_species:
		fig, axes = plt.subplots(
			2, 1, sharex=True, figsize=(7.5, 7.0), constrained_layout=True
		)
		ax_main, ax_zoom = axes
	else:
		fig, ax_main = plt.subplots(1, 1, figsize=(7.5, 4.8), constrained_layout=True)
		ax_zoom = None

	for name, vals, color, style in series:
		ax_main.plot(z_mm, vals, style, color=color, label=name, linewidth=1.8, markersize=5)
	ax_main.set_ylabel("Mole fraction [-]")
	ax_main.set_title(
		"Freeboard mole-fraction profile at T = %.0f K, p = %.3g bar, L = %.3g mm"
		% (t_k, p_bar, l_mm)
	)
	ax_main.grid(True, alpha=0.35)
	ax_main.legend(loc="best")

	if ax_zoom is not None:
		ymin = min(min(vals) for _, vals, _, _ in zoom_species)
		ymax = max(max(vals) for _, vals, _, _ in zoom_species)
		pad = max(1e-6, 0.08 * max(ymax - ymin, 1e-6))
		for name, vals, color, style in zoom_species:
			ax_zoom.plot(
				z_mm, vals, style, color=color, label=name, linewidth=1.8, markersize=5
			)
		ax_zoom.set_ylabel("Zoomed y [-]")
		ax_zoom.set_xlabel("Depth from freeboard top [mm]")
		ax_zoom.set_ylim(ymin - pad, ymax + pad)
		ax_zoom.grid(True, alpha=0.35)
		ax_zoom.legend(loc="best")
	else:
		ax_main.set_xlabel("Depth from freeboard top [mm]")

	_report_note("Plotting completed")
	plt.ion()
	plt.show()


extpy.registermethod(freeboard_plot_profile)
