
# Utils to parse and plot data

import os
import re
import pandas as pd

execution_stat_names = {
    # Stats regarding general execution
    'pps': 'propagations',
    'dcs': 'decitions',
    'cfs': 'conflicts',
    'rst': 'restarts',
    
    # Stats regarding Clause sharing
    'prod': 'produced',
    'recv': 'recieved',
	'intim': 'internally imported',
}

inner_stat_names = {
    'prod': 'produced',
    'recv': 'recieved',
	'flt': 'filtered',
	'adm': 'admitted',
    'digd': 'digested',
	'drp': 'dropped',
}

def print_grouped_stats(gs):
    for key in gs:
        print(key, ':')
        for line in gs[key]:
            print(line)
        print()

def print_dict(h, prefix=''):
    for k,v in h.items():
        if type(v) == dict:
            print(f"{prefix}{k}:")
            print_dict(v, prefix='    ')
            continue
        if type(v) == tuple:
            print(f"{prefix}{k}:", f"{v[0]}/{v[1]}")
            continue
        print(f"{prefix}{k}:", v)

def count_occurences(l):
    d = {}
    for e in l:
        if e not in d: d[e] = 1
        else: d[e] += 1
    return d

def get_max_mem(log):
    # Mallob style output
    globmems = re.findall('globmem=(.*)GB', log)
    if globmems:
        return max([float(x) for x in globmems])
    # Gimsatul style output
    globmem = re.search('c maximum-resident-set-size:( )+([0-9]+.[0-9]+)( bytes)?', log)
    if globmem:
        max_mem = float(globmem.groups()[1]) / 1024
        # Kissat style output
        if ' bytes' in globmem.groups():
            max_mem /= 1024 * 1024
        return max_mem
    return 0

def get_all_mem(log):
    globmems = re.findall('globmem=(.*)GB', log)
    if not globmems: globmems = [0]
    return [float(x) for x in globmems]

def get_result(log):
    res = 0
    if re.search('s SATISFIABLE', log):
        res = 1
    elif re.search('s UNSATISFIABLE', log):
        res = -1
    return res

def get_runtime(log):
    runtime = 0.
    # Mallob style output
    s = re.search("RESPONSE_TIME #[0-9]+ ([0-9]+.[0-9]+)", log)
    if s:
        runtime = float(s.groups()[0])
        return runtime
    # Gimsatul style output
    s = re.search("c wall-clock-time:( )+([0-9]+.[0-9]+)", log)
    if s:
        runtime = float(s.groups()[1])
        return runtime
    # Kissat style output
    s = re.search("c process-time:(.)+ ([0-9]+.[0-9]+) seconds", log)
    if s:
        runtime = float(s.groups()[1])
        return runtime
    return 0

def get_name(dir):
    try:
        file = open(f"{dir}/instance.txt", 'r')
        return file.read().split('/')[-1].replace('\n','')
    except FileNotFoundError:
        file = open(f"{dir}/0/log.0", 'r')
        log = file.read()
        return re.search("-mono=(.*) -mono-app", log).groups()[0].split('/')[-1]

def get_grouped_stats(path):
    dirs = next(os.walk(path))[1]
    grouped_end_lines = {}
    for dir in dirs:
        log = open(f"{path}/{dir}/subproc.{dir}").read()
        end_lines = re.findall('<#([0-9]+)>( END .*)\n', log)
        for grouped_line in end_lines:
            job_id = grouped_line[0]
            if not job_id in grouped_end_lines:
                grouped_end_lines[job_id] = []
            if 'c' in grouped_line[1].split(' '):
                grouped_line[1] = grouped_line[1][grouped_line.index('c')]
            grouped_end_lines[job_id].append(grouped_line[1])
    return grouped_end_lines

def get_kissat_style_name(log):
    return re.search('c   /hppfs/work/pn72pu/di97xah/track_2024/(.*)\n', log).groups()[0]

def add_stats(stat1, stat2):
    if stat1 == {} or type(stat1) != dict:
        return stat2
    if stat2 == {} or type(stat2) != dict:
        return stat1
    new_stat = {}
    for key in stat1:
        if type(stat1[key]) == tuple:
            new_stat[key] = (stat1[key][0] + stat2[key][0], stat1[key][1] + stat2[key][1])
        elif type(stat1[key]) == dict:
            new_stat[key] = add_stats(stat1[key], stat2[key])
        else:
            new_stat[key] = stat1[key] + stat2[key]
    return new_stat

def sum_up_stats(stat_list):
    summed_stats = {}
    i = 0
    for stat in stat_list:
        summed_stats = add_stats(summed_stats, stat)
    return summed_stats

def normalize_stats(stats, procs):
    new_stats = {}
    for k,v in stats.items():
        if type(v) == tuple:
            new_stats[k] = (int(v[0] / procs), int(v[1] / procs))
            continue
        new_stats[k] = int(v / procs)
    return new_stats

def parse_overview(path, prefix):
    dirs = next(os.walk(path))[1]
    data = {f"name":[],
            f"{prefix}_result":[],
            f"{prefix}_memory":[],
            f"{prefix}_runtime":[]}
    for name in dirs:
        try:
            file = open(f"{path}/{name}/out_file", 'r')
            log = file.read().__str__()
        except FileNotFoundError:
            file = open(f"{path}/{name}/0/log.0")
            log = file.read().__str__()
            name = get_name(f"{path}/{name}")

        # add data
        data[f"name"].append(name)
        data[f"{prefix}_result"].append(get_result(log))
        data[f"{prefix}_memory"].append(get_max_mem(log))
        data[f"{prefix}_runtime"].append(get_runtime(log))
    
    df = pd.DataFrame(data)
    df = df.merge(parse_stats(path, prefix), on='name', how='left')

    return df

def parse_mem(path, prefix):
    dirs = next(os.walk(path))[1]
    data = {f"name":[],
            f"{prefix}_result":[],
            f"{prefix}_mem_per_sec":[],
            f"{prefix}_runtime":[]}
    for name in dirs:
        try:
            file = open(f"{path}/{name}/out_file", 'r')
            log = file.read().__str__()
        except FileNotFoundError:
            file = open(f"{path}/{name}/0/log.0")
            log = file.read().__str__()
            name = get_name(f"{path}/{name}")

        # add data
        data[f"name"].append(name)
        data[f"{prefix}_result"].append(get_result(log))
        data[f"{prefix}_mem_per_sec"].append(get_all_mem(log))
        data[f"{prefix}_runtime"].append(get_runtime(log))

    return pd.DataFrame(data)

def get_satwp_extraction(log):
    h = {'prepro': 0,
         'base': 0}
    # cound :prepro extracted
    res = re.search('SATWP #([1-9])+:prepro extracted', log)
    if res:
        h['prepro'] += int(res.groups()[0])
    # count :base extracted
    res = re.search('SATWP #([1-9])+:base extracted', log)
    if res:
        h['base'] += int(res.groups()[0])

    return h

def init_execution_stats():
    execution_stats = {}

    for key, stat_name in execution_stat_names.items():
        execution_stats[stat_name] = 0

        if stat_name == execution_stat_names['prod']:
            execution_stats[stat_name] = {
                'produced': 0,
                'filtered': 0,
                'admitted': 0,
                'dropped': 0,
            }

        if stat_name == execution_stat_names['recv']:
            execution_stats[stat_name] = {
                'recieved': 0,
                'filtered': 0,
                'digested': 0,
                'dropped': 0,
            }
            
    execution_stats[execution_stat_names['intim']] = (0,0)

    return execution_stats

def count_satwp_extractions_over_benchmarks(path, prefix):
    dirs = next(os.walk(path))[1]
    h = {f"{prefix}_prepro": 0,
         f"{prefix}_base": 0}
    for name in dirs:
        file = open(f"{path}/{name}/out_file", 'r')
        log = file.read().__str__()
        res = get_satwp_extraction(log)
        h[f"{prefix}_prepro"] += res['prepro']
        h[f"{prefix}_base"] += res['base']

    return h

def accumulate_clenhist_lines(clenhist_lines):
    clenhist_lines = [ s.split(':')[1] if len(s.split(':')) > 1 else '' for s in clenhist_lines ]
    clenhist_lines = [ re.split('[a-z]|[A-Z]|\\(|/|<', s)[0] for s in clenhist_lines ]
    clenhist_lines = [ s.split(' ')[1:] for s in clenhist_lines ]
    clenhist_lines = [ list(filter(lambda x: len(x) > 0, s)) for s in clenhist_lines ]
    
    clenhist_lines = [ list(map(int, x)) for x in clenhist_lines ]
    try:
        max_hist_length = max([ len(l) for l in clenhist_lines ])
    except ValueError:
        max_hist_length = 0
    clenhist_lines = [ l + ([0] * (max_hist_length - len(l))) for l in clenhist_lines ]
    clenhist_lines = [ sum(t) for t in zip(*clenhist_lines) ]
    if clenhist_lines:
        return clenhist_lines

    return []

def accumulate_execution_stat_lines(stat_lines):
    execution_stats = init_execution_stats()
    intim = execution_stat_names['intim']

    for line in stat_lines:
        line = line.split(')c')[0].split(' a <')[0].split('intc')[0].split('dcsim')[0].split(' ')[3:]
        i = -1
        while True:
            i += 1
            if i >= len(line):
                break
            stat = line[i]
            
            # not a stat
            if '+' in stat:
                continue

            # spezial case for nested stats
            if 'prod' in stat:
                inner_stats = line[i:i+4]
                prod = execution_stat_names['prod']
                for inner_stat in inner_stats:
                    try:
                        stat_key, value = inner_stat.replace('(','').replace(')','').split(':')
                        execution_stats[prod][inner_stat_names[stat_key]] += int(value)
                    except:
                        pass
                i += 3
                continue

            if 'recv' in stat:
                inner_stats = line[i:i+4]
                recv = execution_stat_names['recv']
                for inner_stat in inner_stats:
                    try:
                        stat_key, value = inner_stat.replace('(','').replace(')','').split(':')
                        execution_stats[recv][inner_stat_names[stat_key]] += int(value)
                    except:
                        pass
                i += 3
                continue

            # spezial case for internally importet
            if 'intim' in stat:
                try:
                    values = [ int(x) for x in stat.split(':')[1].split('/') ]
                    old_intim = execution_stats[intim]
                    execution_stats[intim] = (old_intim[0] + values[0], old_intim[1] + values[1])
                except:
                    pass
                continue
            
            if len(stat.split(':')) < 2:
                continue
            try:
                stat_key, value = stat.split(':')
                execution_stats[execution_stat_names[stat_key]] += int(value)
            except:
                pass

    return execution_stats

def count_satwp_extractions_over_benchmarks(path, prefix):
    dirs = next(os.walk(path))[1]
    h = {f"{prefix}_prepro": 0,
         f"{prefix}_base": 0}
    for name in dirs:
        file = open(f"{path}/{name}/out_file", 'r')
        log = file.read().__str__()
        res = get_satwp_extraction(log)
        h[f"{prefix}_prepro"] += res['prepro']
        h[f"{prefix}_base"] += res['base']

    return h

def parse_serial_overview(path, prefix):
    dirs = next(os.walk(path))[1]
    data = {f"name":[],
            f"{prefix}_result":[],
            f"{prefix}_memory":[],
            f"{prefix}_runtime":[]}
    for name in dirs:
        file = open(f"{path}/{name}/rw")
        log = file.read().__str__()
        name = get_kissat_style_name(log)

        # add data
        data[f"name"].append(name)
        data[f"{prefix}_result"].append(get_result(log))
        data[f"{prefix}_memory"].append(get_max_mem(log))
        data[f"{prefix}_runtime"].append(get_runtime(log))
    
    return pd.DataFrame(data)

# TODO: Some instances have no stats printed. e.g. sMUC_logs_2node_1p_48t/107/1/
def parse_stats(path, prefix):
    dirs = next(os.walk(path))[1]
    data = {f"name":[],
        f"{prefix}_clenhist_prod":[],
        f"{prefix}_clenhist_digd":[],
        f"{prefix}_general_stats":[]
    }
    for name in dirs:
        try:
            grouped_stats = get_grouped_stats(f"{path}/{name}")
        except:
            continue

        clenhist_prod_tmp = []
        clenhist_digd_tmp = []
        meta_stats_tmp = []
        for k in grouped_stats:
            for line in grouped_stats[k]:
                if 'clenhist prod total' in line:
                    clenhist_prod_tmp.append(line)
                if 'clenhist digd total' in line:
                    clenhist_digd_tmp.append(line)
                if re.search(' S[0-9]+ pps:', line):
                    meta_stats_tmp.append(line)

        data['name'].append(get_name(f"{path}/{name}"))
        data[f"{prefix}_clenhist_prod"].append(accumulate_clenhist_lines(clenhist_prod_tmp))
        data[f"{prefix}_clenhist_digd"].append(accumulate_clenhist_lines(clenhist_digd_tmp))
        data[f"{prefix}_general_stats"].append(accumulate_execution_stat_lines(meta_stats_tmp))

    return pd.DataFrame(data)
