
# Utils to parse and plot data

import os
import re
import pandas as pd

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
    
    return pd.DataFrame(data)

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
