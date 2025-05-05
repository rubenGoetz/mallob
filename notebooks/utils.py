
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
    globmem = re.search('c maximum-resident-set-size:( )+([0-9]+.[0-9]+)', log)
    if globmem:
        return float(globmem.groups()[1]) / 1024
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
    return 0

def parse_overview(path, prefix):
    dirs = next(os.walk(path))[1]
    data = {f"name":[],
            f"{prefix}_result":[],
            f"{prefix}_memory":[],
            f"{prefix}_runtime":[]}
    for name in dirs:
        file = open(f"{path}/{name}/out_file", 'r')
        log = file.read().__str__()

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
        file = open(f"{path}/{name}/out_file", 'r')
        log = file.read().__str__()

        # add data
        data[f"name"].append(name)
        data[f"{prefix}_result"].append(get_result(log))
        data[f"{prefix}_mem_per_sec"].append(get_all_mem(log))
        data[f"{prefix}_runtime"].append(get_runtime(log))

    return pd.DataFrame(data)
