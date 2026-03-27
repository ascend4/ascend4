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
		return title_fn(entries[0])
	return ""

COLOR_CYCLE = ['b','r','g','y','c','m','k']
