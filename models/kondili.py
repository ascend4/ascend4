#!/usr/bin/env python3
"""Solve and report the original Kondili batch scheduling cases.

./a4 script models/kondili.py --solver HiGHS --case original --output kondili.png
The ASCEND instance is the sole source of recipes, equipment and schedules.
"""
import argparse
import math
from pathlib import Path
import sys


def named(array, key):
    """The current ascpy [] operator accepts integer indices only."""
    for c in array.getChildren():
        if str(c.getName()) == str(key):
            return c
    raise ValueError(f'Missing array element {key}')


def read_results(model):
    """Copy solved values into plain Python data, with kg and hours for display."""
    d, g, p = model.data, model.grid, model.plant
    points = sorted(g.point.getSetValue())
    times = [g.t[t].getRealValue()/3600 for t in points]
    recipes, stocks, units = {}, {}, {}
    for i in map(str,d.task.getSetValue()):
        r = named(d.recipe,i)
        recipes[i] = dict(duration=r.duration.getRealValue()/3600,
            inputs={s: named(r.rin,s).getRealValue() for s in map(str,r.feed.getSetValue())},
            outputs={s: (named(r.rout,s).getRealValue(), named(r.delay,s).getRealValue()/3600)
                     for s in map(str,r.product.getSetValue())})
    for s in map(str,d.state.getSetValue()):
        v = named(d.stock,s)
        stocks[s] = dict(initial=v.initial.getRealValue(),
            capacity=v.capacity.getRealValue() if v.limited.getBoolValue() else None,
            price=v.price.getRealValue(), values=[named(p.v,s).stock[t].getRealValue() for t in points])
    for j in map(str,d.unit.getSetValue()):
        batches = []
        equipment = named(d.equipment,j)
        unit = named(p.u,j)
        for i in map(str,equipment.task.getSetValue()):
            op = named(unit.op,i)
            for k in op.launch.getSetValue():
                w, b = op.w[k].getRealValue(), op.b[k].getRealValue()
                if not math.isfinite(w) or min(abs(w), abs(w-1)) > 1e-6:
                    raise ValueError(f"Non-binary start: {j}/{i}/{k}")
                bmin = named(equipment.bmin,i).getRealValue()
                bmax = named(equipment.bmax,i).getRealValue()
                if not math.isfinite(b) or b < bmin*w-1e-6 or b > bmax*w+1e-6:
                    raise ValueError(f"Invalid batch size: {j}/{i}/{k}")
                if w > 0.5:
                    batches.append(dict(task=i, start=g.t[k].getRealValue()/3600, mass=max(0,b)))
        units[j] = dict(batches=sorted(batches, key=lambda b: b['start']),
                        held=[unit.held[t].getRealValue() for t in points])
    result = dict(times=times, recipes=recipes, stocks=stocks, units=units,
                  value=p.value.getRealValue())
    validate_results(result)
    return result


def validate_results(result, tol=1e-5):
    """Independently reconstruct batch events, stock and equipment mass balances."""
    times, recipes, stocks, units = (result[k] for k in ('times','recipes','stocks','units'))
    if not times or times[0] != 0 or any(not math.isfinite(t) for t in times):
        raise ValueError("Invalid time grid")
    if any(b <= a for a,b in zip(times,times[1:])):
        raise ValueError("Time grid must increase")
    for r in recipes.values():
        if not math.isfinite(r['duration']) or r['duration'] <= 0:
            raise ValueError('Invalid recipe duration')
        for fractions in (r['inputs'], {s:f for s,(f,delay) in r['outputs'].items()}):
            if (not fractions or any(s not in stocks or not math.isfinite(f) or f <= 0 for s,f in fractions.items())
                    or abs(sum(fractions.values())-1) > 1e-10):
                raise ValueError('Invalid mass fractions or material state')
        delays = [delay for f,delay in r['outputs'].values()]
        if any(not math.isfinite(d) or d <= 0 or d > r['duration'] for d in delays) or max(delays) != r['duration']:
            raise ValueError('Invalid product release times')
    def tick(t):
        matches = [k for k,x in enumerate(times) if abs(t-x) < 1e-8]
        if len(matches) != 1:
            raise ValueError("Batch event is off the time grid or outside the horizon")
        return matches[0]
    delta = {s: [0.0]*len(times) for s in stocks}
    for j,u in units.items():
        change = [0.0]*len(times)
        finish = 0.0
        for b in sorted(u['batches'], key=lambda b: b['start']):
            r = recipes[b['task']]
            if not math.isfinite(b['mass']) or b['mass'] < 0:
                raise ValueError("Invalid batch mass")
            if b['start'] < finish-1e-8:
                raise ValueError(f"Overlapping batches on {j}")
            finish = b['start']+r['duration']
            tick(finish)
            k = tick(b['start'])
            change[k] += b['mass']
            for s,f in r['inputs'].items():
                delta[s][k] -= f*b['mass']
            for s,(f,delay) in r['outputs'].items():
                q = tick(b['start']+delay)
                delta[s][q] += f*b['mass']
                change[q] -= f*b['mass']
        held = 0.0
        if len(u['held']) != len(times):
            raise ValueError("Missing equipment inventory samples")
        for k in range(len(times)):
            held += change[k]
            if not math.isfinite(held) or not math.isfinite(u['held'][k]) or abs(held-u['held'][k]) > tol or held < -tol:
                raise ValueError(f"Equipment mass balance failed for {j}")
        if abs(held) > tol:
            raise ValueError(f"Unfinished material in {j}")
    value = 0.0
    for s,v in stocks.items():
        mass = v['initial']
        if not math.isfinite(mass) or mass < 0 or not math.isfinite(v['price']):
            raise ValueError('Invalid initial stock or price')
        if v['capacity'] is not None and (not math.isfinite(v['capacity']) or v['capacity'] < 0):
            raise ValueError('Invalid storage capacity')
        if len(v['values']) != len(times):
            raise ValueError("Missing stock samples")
        for k in range(len(times)):
            mass += delta[s][k]
            if not math.isfinite(mass) or not math.isfinite(v['values'][k]) or abs(mass-v['values'][k]) > tol or mass < -tol:
                raise ValueError(f"Stock balance failed for {s}")
            if v['capacity'] is not None and mass > v['capacity']+tol:
                raise ValueError(f"Storage capacity exceeded for {s}")
        value += mass*v['price']
    if not math.isfinite(result['value']) or abs(value-result['value']) > tol:
        raise ValueError("Terminal value does not match inventories")


def plot_results(result, solver=None):
    validate_results(result)
    import matplotlib.pyplot as plt
    from matplotlib.patches import Patch
    recipes, units = result['recipes'], result['units']
    names = sorted(recipes)
    palette = plt.get_cmap('Set2')
    colors = {i: palette(k % 8) for k,i in enumerate(names)}
    fig = plt.figure(figsize=(13,11), layout='constrained')
    rows = math.ceil(len(result['stocks'])/3)
    layout = fig.add_gridspec(1+rows,3, height_ratios=[2.3]+[1]*rows)
    ax = fig.add_subplot(layout[0,:])
    for lane,j in enumerate(sorted(units)):
        for b in units[j]['batches']:
            if b['mass'] < 1e-6:  # Cost-free zero-mass starts have no physical batch.
                continue
            r = recipes[b['task']]
            ax.barh(lane,r['duration'],left=b['start'],height=.65,color=colors[b['task']],edgecolor='black')
            ax.text(b['start']+r['duration']/2,lane,f"{b['mass']:.3g} kg",ha='center',va='center',fontsize=8)
            for f,delay in r['outputs'].values():
                if delay < r['duration']:
                    ax.plot(b['start']+delay,lane+.3,marker='v',color='black',markersize=5)
    ax.set_yticks(range(len(units)),[j.replace('_',' ') for j in sorted(units)])
    ax.set(xlim=(0,result['times'][-1]),xlabel='Elapsed time / h',title='Equipment schedule | bar labels: batch mass; ▼: early discharge')
    ax.legend(handles=[Patch(facecolor=colors[i],label=i.replace('_',' ')) for i in names],ncol=5,loc='upper center',bbox_to_anchor=(.5,1.3))
    for k,(s,v) in enumerate(sorted(result['stocks'].items())):
        ax = fig.add_subplot(layout[1+k//3,k%3])
        ax.step(result['times'],v['values'],where='post',color='#336699')
        ax.plot(0,v['initial'],'o',mfc='white',mec='#336699',ms=4)
        ax.plot(result['times'][-1],v['values'][-1],'o',color='#336699',ms=3)
        if v['capacity'] is not None:
            ax.axhline(v['capacity'],color='#bb4444',ls='--',label='Capacity')
            ax.legend(fontsize=7)
        ax.set(title=s.replace('_',' '),xlabel='Time / h',ylabel='Inventory / kg',xlim=(0,result['times'][-1]*1.02))
        ax.set_ylim(bottom=0)
    bc = result['stocks'].get('Int_BC',{}).get('capacity')
    case = f" | BC storage: {bc:g} kg" if bc is not None else ''
    fig.suptitle(f"Kondili batch process{case} | terminal value {result['value']:.3f} currency units"
                 + (f" | {solver}" if solver else ''))
    return fig


def print_results(result):
    print(f"Terminal value: {result['value']:.6f} currency units")
    print('Unit          Task          Start/h  Finish/h   Batch/kg   Discharges (state, hour, kg)')
    for j,u in sorted(result['units'].items()):
        for b in u['batches']:
            if b['mass'] < 1e-6:
                continue
            r = result['recipes'][b['task']]
            releases = '; '.join(f"{s}, {b['start']+delay:g}, {f*b['mass']:.4g}" for s,(f,delay) in r['outputs'].items())
            print(f"{j:13} {b['task']:13} {b['start']:7g} {b['start']+r['duration']:9g} {b['mass']:10.4f}   {releases}")
    print('Final inventories / kg:')
    for s,v in sorted(result['stocks'].items()):
        print(f"  {s:12} {v['values'][-1]:12.6f}")


def solve_case(solver='HiGHS', case='original'):
    import ascpy
    lib = ascpy.Library()
    name = 'kondili' if case == 'original' else 'kondili_no_bc_storage'
    try:
        typ = lib.findType(name)
    except RuntimeError:
        lib.load(str(Path(__file__).with_suffix('.a4c')))
        typ = lib.findType(name)
    sim = typ.getSimulation('schedule',False)
    sim.setSolver(ascpy.Solver(solver))
    params = sim.getParameters()
    for p in params:
        if p.getName() == 'threads': p.setIntValue(1)
        elif p.getName() in ('mip_rel_gap','mip_abs_gap'): p.setRealValue(0.0)
    sim.setParameters(params)
    sim.solve(ascpy.Solver(solver),ascpy.SolverReporter())
    if not sim.getStatus().isConverged():
        raise RuntimeError(f'{solver} did not converge')
    sim.run(next(m for m in typ.getMethods() if str(m.getName()) == 'self_test'))
    return read_results(sim.getModel())


def kondili_plot(self):
    """Plot the solved model from the GUI; run Solve before this method."""
    import extpy
    browser = extpy.getbrowser()
    if browser is None or browser.sim.isSolveDirty() or not browser.sim.getStatus().isConverged():
        raise RuntimeError('Solve the model successfully before plotting')
    result = read_results(self)
    import loading
    loading.load_matplotlib(throw=True)
    import matplotlib.pyplot as plt
    fig = plot_results(result,str(browser.sim.getSolver().getName()))
    plt.show(block=False)
    return fig


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--solver',choices=('HiGHS','Gurobi'),default='HiGHS')
    parser.add_argument('--case',choices=('original','no-bc-storage'),default='original')
    parser.add_argument('--output',type=Path,default=Path('kondili.png'),help='PNG/SVG/PDF chart (overwrites this file)')
    parser.add_argument('--show',action='store_true')
    args = parser.parse_args(argv)
    try:
        import matplotlib
        if not args.show: matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        result = solve_case(args.solver,args.case)
        print_results(result)
        fig = plot_results(result,args.solver)
        try:
            fig.savefig(args.output,dpi=160)
            print(f'Chart: {args.output.resolve()}')
            if args.show: plt.show()
        finally:
            plt.close(fig)
    except (ImportError,RuntimeError,ValueError,OSError) as exc:
        print(f'kondili: {exc}',file=sys.stderr)
        return 1
    return 0


if 'extpy' in sys.modules:
    sys.modules['extpy'].registermethod(kondili_plot)
elif __name__ == '__main__':
    sys.exit(main())
