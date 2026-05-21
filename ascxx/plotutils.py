def group_series(series, group_key_fn):
	grouped = {}
	group_order = []
	for entry in series:
		g = group_key_fn(entry)
		if g not in grouped:
			grouped[g] = []
			group_order.append(g)
		grouped[g].append(entry)
	return grouped, group_order

def group_ylabel(entries, unit_name_fn, title_fn):
	units = sorted(set([u for u in [unit_name_fn(entry) for entry in entries] if u != ""]))
	if len(entries) > 1:
		if len(units) == 1:
			return "[%s]" % units[0]
		return ""
	if len(entries) == 1:
		title = title_fn(entries[0])
		if len(units) == 1 and ("[%s]" % units[0]) not in title:
			return "%s / [%s]" % (title, units[0])
		return title
	return ""

COLOR_CYCLE = [
	"#1f77b4",
	"#d62728",
	"#2ca02c",
	"#ff7f0e",
	"#9467bd",
	"#17becf",
	"#8c564b",
	"#7f7f7f",
]

def plot_figure_size(n_axes, n_series):
	width = 10.5
	height = max(4.2, 1.75 * max(1, n_axes) + 0.15 * min(max(0, n_series), 10))
	return (width, height)

def style_time_axis(ax):
	ax.grid(True, color="0.86", linewidth=0.8)
	ax.margins(x=0.01, y=0.10)
	ax.tick_params(axis="both", labelsize=9)
	for spine in ax.spines.values():
		spine.set_color("0.35")

def add_series_legend(ax, n_items):
	if n_items <= 1:
		return None
	ncols = 1 if n_items <= 8 else 2
	leg = ax.legend(
		loc="center left",
		bbox_to_anchor=(1.01, 0.5),
		borderaxespad=0.0,
		frameon=True,
		fontsize=9,
		ncol=ncols,
	)
	if leg is not None:
		leg.get_frame().set_alpha(0.92)
	return leg

def finish_time_series_layout(fig, axes, has_outside_legend):
	if axes:
		fig.align_ylabels(axes)
	right = 0.78 if has_outside_legend else 0.97
	fig.subplots_adjust(left=0.12, right=right, top=0.96, bottom=0.10, hspace=0.14)

def plot_grouped_time_series(plt, x_values, groups, x_label, marker=None, event_indices=None, make_legend_draggable=None):
	n_series = sum(len(group["series"]) for group in groups)
	fig, axes = plt.subplots(
		len(groups),
		1,
		squeeze=False,
		sharex=True,
		figsize=plot_figure_size(len(groups), n_series),
	)
	axes = [ax[0] for ax in axes]
	event_indices = event_indices or []
	has_outside_legend = any(len(group["series"]) > 1 for group in groups)

	for ax_index, (ax, group) in enumerate(zip(axes, groups)):
		for fallback_index, series in enumerate(group["series"]):
			color_index = series.get("color_index", fallback_index)
			color = COLOR_CYCLE[color_index % len(COLOR_CYCLE)]
			plot_kwargs = {
				"color": color,
				"linewidth": 1.6,
				"label": series["label"],
			}
			if marker:
				plot_kwargs["marker"] = marker
				plot_kwargs["markersize"] = 4
			ax.plot(
				x_values,
				series["values"],
				"-",
				**plot_kwargs
			)
			if event_indices:
				ax.plot(
					[x_values[i] for i in event_indices],
					[series["values"][i] for i in event_indices],
					"o",
					ms=5,
					mfc="none",
					mec=color,
					linestyle="None",
				)
		ax.set_ylabel(group["ylabel"], labelpad=20)
		style_time_axis(ax)
		leg = add_series_legend(ax, len(group["series"]))
		if make_legend_draggable is not None:
			make_legend_draggable(leg)
		if ax_index + 1 != len(axes):
			plt.setp(ax.get_xticklabels(), visible=False)

	axes[-1].set_xlabel(x_label)
	finish_time_series_layout(fig, axes, has_outside_legend)
	return fig, axes
