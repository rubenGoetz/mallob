
import itertools

from matplotlib import rc, ticker
from matplotlib import pyplot as plt

###################
## Generic Plots ##
###################

def plot_square(df,
                xaxis='runtime_solve',
                yaxis='runtime_check',
                xlabel='solving time in s',
                ylabel='checking time in s',
                title='X Nodes',
                mark='.',
                filename=None):
    fig = plt.figure()
    ax = fig.add_subplot()
    ax.plot([0, 1], [0, 1], transform=ax.transAxes, color='grey')
    plt.plot(df[xaxis],
             df[yaxis],
             mark)
    _min = min(min(df[xaxis]),
              min(df[yaxis]))
    _max = max(max(df[xaxis]),
              max(df[yaxis]))
    plt.axis([_min, _max] * 2)
    plt.xscale('log')
    plt.yscale('log')
    ax.set_aspect(1.0/ax.get_data_ratio(), adjustable='box')
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.grid()
    if filename:
        plt.savefig(filename, bbox_inches='tight')
    plt.show()

####################
## Specific Plots ##
####################

def plot_pal_max_runtimes(df,
                          fp_name='runtime_first_pass',
                          rr_name='runtime_reroute',
                          lp_name='runtime_last_pass',
                          overall_name='runtime_check',
                          label_prefix='max ',
                          filename=None):
    df_sorted = df.sort_values(overall_name)
    
    ax = plt.axes()
    ax.grid(axis='y')
    ax.set_axisbelow(True)
    
    plt.bar(range(0,len(df_sorted)),
            [ float(x)+float(y)+float(z) for x,y,z in zip(df_sorted[fp_name], df_sorted[rr_name], df_sorted[lp_name]) ],
            width=.5,
            label=f"{label_prefix}last_pass",
            color='grey')
    plt.bar(range(0,len(df_sorted)),
            [ float(x)+float(y) for x,y in zip(df_sorted[fp_name], df_sorted[rr_name]) ],
            width=.5,
            label=f"{label_prefix} reroute",
            color='red')
    plt.bar(range(0,len(df_sorted)),
            [ float(x) for x in df_sorted[fp_name] ],
            width=.5,
            label=f"{label_prefix} first_pass",
            color='blue')
    
    plt.xlabel('instance')
    plt.ylabel('runtime in s')
    plt.legend()
    if filename:
        plt.savefig(filename, bbox_inches='tight')
    plt.show()

def plot_data_sizes(df,
                    proof_name='proof_size',
                    proxy_name='proxy_size',
                    import_name='import_size',
                    xlabel='instance',
                    ylabel='read data in Byte',
                    title='',
                    filename=None):
    df_sorted = df.sort_values(proof_name)
    
    ax = plt.axes()
    ax.grid(axis='y')
    ax.set_axisbelow(True)
    
    plt.bar(range(0,len(df_sorted)),
            df_sorted[proof_name],
            width=.5,
            label='proof',
            color='grey')
    plt.bar(range(0,len(df_sorted)),
            df_sorted[proxy_name],
            width=.5,
            label='proxy',
            color='red')
    plt.bar(range(0,len(df_sorted)),
            df_sorted[import_name],
            width=.5,
            label='import',
            color='blue')
    
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.legend()
    plt.yscale('log')
    if filename:
        plt.savefig(filename, bbox_inches='tight')
    plt.show()

#####################
## SAT_2026_PALRUP ##
#####################

def plot_solved_over_time(dfs, labels,
                          column_to_plot='runtime_solve',
                          figsize=[2.75, 2.75],
                          line_styles=['-', '--'],
                          filename=None):
    if len(dfs) != len(labels):
        raise ValueError("dfs and labels need to be of same length")
    line_style = itertools.cycle(line_styles)
    rc('text', usetex=True)
    rc('font', family='serif')

    plt.figure(figsize=figsize)
    for i in range(0, len(dfs)):
        df = dfs[i]
        label = labels[i]

        df.sort_values(column_to_plot, inplace=True)
        plt.plot(df[column_to_plot],
                 range(0, len(df)),
                 next(line_style),
                 label=label)
    
    plt.xlabel('Runtime $t$ in s')
    plt.ylabel('Solved instances in $\leq t$')
    plt.axis('square')
    plt.grid()
    plt.legend()
    plt.xlim(left=0, right=300)
    plt.ylim(bottom=0)
    plt.tight_layout()
    if filename:
        plt.savefig(filename)
    plt.show()

# plot_solved_over_time(list(reversed(dfs)), labels=list(reversed(labels)), filename='raw_solving_times.pdf')

def plot_checker_components_runtime(dfs,
                                    marks=['+', '.', 'x', '*'],
                                    overall_col='runtime_all',
                                    comp_cols=['avg_runtime_first_pass',
                                               'avg_runtime_reroute',
                                               'avg_runtime_last_pass'],
                                    labels=['local checking',
                                            'redistribution',
                                            'confirmation'],
                                    figsize=[5.5, 2.75],
                                    title='',
                                    filename=None):
    if len(comp_cols) != len(labels):
        raise ValueError("comp_cols and labels must have same length")

    mark_style = itertools.cycle(marks)
    rc('text', usetex=True)
    rc('font', family='serif')

    fig, axs = plt.subplots(1, len(dfs), figsize=figsize)
    #df_sorted = df.sort_values(overall_col)
    
    for i in range(0, len(dfs)):
        df = dfs[i][0].sort_values(overall_col)
        title= dfs[i][1]
        ax = axs[i]
        xrange = range(0, len(df))

        summed_runtime = [ x+y+z+a+b+c for x,y,z,a,b,c in zip(df["avg_runtime_first_pass"],
                                                          df["avg_runtime_reroute"],
                                                          df["avg_runtime_last_pass"],
                                                          df["avg_waittime_first_pass"],
                                                          df["avg_waittime_reroute"],
                                                          df["avg_waittime_last_pass"])]

        ax.plot(xrange,
                summed_runtime,
                next(mark_style),
                label='WC time',
                markersize=3)
        for (col, label) in zip(comp_cols, labels):
            ax.plot(xrange,
                     df[col],
                     next(mark_style),
                     label=label,
                     markersize=3)

        #ax.xlim(left=0, right=len(df))
        #ax.ylim(bottom=0)
        if i == 0:
            ax.set(ylabel='Runtime in s')
        ax.set(title=title, yscale='log', xlim=[0, len(df)], ylim=0)
        ax.grid(axis='y')
        ax.tick_params(axis='x', which='both', bottom=False, top=False, labelbottom=False)
        ax.set_box_aspect(1)


    fig.legend(labels=['WC time']+labels, loc='center left', bbox_to_anchor=(1, 0.5))
    axs[1].set(xlabel='Instances sorted by WC time')
    fig.tight_layout()
    if filename:
       fig.savefig(filename, bbox_inches='tight')
    fig.show()

# plot_checker_components_runtime(dfs, filename='runtime_checking_stages.pdf')

def plot_tight_square(dfs,
                      labels='',
                xaxis='runtime_solve',
                xlabel='solving time in s',
                ylabel='checking time in s',
                marks=['+', '.', 'x', '*'],
                colors=['blue', 'orange', 'green', 'red'],
                title='X Nodes',
                figsize=[5.5, 2.75],
                filename=None):
    if len(dfs) != len(labels):
        raise ValueError("dfs and labels must have same length")
    
    mark_style = itertools.cycle(marks)
    colors = itertools.cycle(colors)
    rc('text', usetex=True)
    rc('font', family='serif')

    _min = 3000
    _max = 0

    fig = plt.figure(figsize=figsize)
    ax = fig.add_subplot()
    ax.plot([0, 1], [0, 1], transform=ax.transAxes, color='grey')

    for df, label in zip(dfs, labels):
        check_time = [ x+y+z+a+b+c for x,y,z,a,b,c in zip(df["max_runtime_first_pass"],
                                                          df["max_runtime_reroute"],
                                                          df["max_runtime_last_pass"],
                                                          df["max_waittime_first_pass"],
                                                          df["max_waittime_reroute"],
                                                          df["max_waittime_last_pass"]) ] if "max_runtime_first_pass" in df else df['runtime_check']

        plt.plot(df[xaxis],
                 check_time,
                 next(mark_style),
                 label=label,
                 color=next(colors),
                 markersize=5)
        _min = min(min(df[xaxis]),
                  min(check_time), _min)
        _max = max(max(df[xaxis]),
                  max(check_time), _max)
    
    print("_min:", _min, "_max:", _max)
    #ax.axis([_min, _max] * 2)
    ax.axis([_min, 300] * 2)
    plt.xscale('log')
    plt.yscale('log')
    ax.set_aspect(1.0/ax.get_data_ratio(), adjustable='box')
    plt.title(title)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.legend(loc='center left', bbox_to_anchor=(1, 0.5))
    plt.grid()
    if filename:
        plt.savefig(filename, bbox_inches='tight')
    plt.show()

# plot_tight_square(dfs_mono + dfs,
#                   title='',
#                   marks=['+', '*', '.', '+', 'x'],
#                   colors=['tab:blue', 'tab:cyan'] + ['tab:green', 'tab:orange', 'tab:red'],
#                   xaxis='runtime_solve',
#                   labels=['1 node mono', '16 nodes mono', '1 node PaRUP', '16 nodes PaRUP', '64 nodes PaRUP'],
#                   filename='square_plot.pdf')
