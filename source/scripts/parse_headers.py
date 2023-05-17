#!/usr/bin/env python3

import os
from pathlib import Path
import yaml

import cxxheaderparser
from cxxheaderparser.simple import parse_file
import dataclasses
import json

STK = Path('build/deps/stk-install')
STK_INCLUDE = STK / 'include/stk'

def parse():
    ps_list = []
    for f in STK_INCLUDE.iterdir():
        try:
            ps_list.append(parse_file(f))
        except cxxheaderparser.errors.CxxParseError:
            print("ERROR: ", f)
            continue

    ds_list = [dataclasses.asdict(p) for p in ps_list]
    return ds_list

def process(ds):
    d = ds[0]
    d = d['namespace']['namespaces']['stk']
    rs = []

    r = {
        'classname': None,
        'constructors':[],
        'destructor': None,
        'methods':[],
    }

    r['classname'] = d['classes'][0]['class_decl']['typename']['segments'][0]['name']

    for m in d['classes'][0]['methods']:
        if m['access'] == 'public':
            if m['constructor']:            
                cname = m['name']['segments'][0]['name']
                r['constructors'].append(cname)
            elif m['destructor']:
                r['destructor'] = m['name']['segments'][0]['name']
            else:
                mname = m['name']['segments'][0]['name']
                ps = []
                returns = None
                print(mname)
                if 'typename' in m['return_type']:
                    returns = m['return_type']['typename']['segments'][0]['name']
                for p in m['parameters']:
                    # print('\t', p)
                    # print()
                    name = p['name']
                    if 'typename' in p['type']:
                        typ = p['type']['typename']['segments'][0]['name']
                        ps.append(dict(name=name, type=typ))

                r['methods'].append(
                    {
                        'name': mname,
                        'params': ps,
                        'returns': returns
                    }            
                )


ds = parse()
ds = ds[22]

with open('parsed.yml', 'w') as f:
    yml = yaml.dump(ds)
    f.write(yml)
