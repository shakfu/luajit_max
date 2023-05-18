#!/usr/bin/env python3

import os
from pathlib import Path
import yaml
from pprint import pprint

import cxxheaderparser
from cxxheaderparser.simple import parse_file
import dataclasses
import json

STK = Path('build/deps/stk-install')
STK_INCLUDE = STK / 'include/stk'

def parse(name=None):
    ps_list = []
    for f in STK_INCLUDE.iterdir():            
        try:
            if name:
                if f.stem == name:
                    ps_list.append(parse_file(f))
            else:
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
                c = dict(
                    name = m['name']['segments'][0]['name'],
                    params = [],
                    # doc = m['doxygen']
                )

                for p in m['parameters']:
                    name = p['name']
                    if 'typename' in p['type']:
                        typ = p['type']['typename']['segments'][0]['name']
                        c['params'].append(dict(name=name, type=typ, ref=False))
                    elif 'ref_to' in p['type']:
                        typ = p['type']['ref_to']['segments'][0]['name']
                        c['params'].append(dict(name=name, type=typ, ref=True))


                r['constructors'].append(c)
            elif m['destructor']:
                r['destructor'] = m['name']['segments'][0]['name']
            else:
                f = dict(
                    name = m['name']['segments'][0]['name'],
                    params = [],
                    returns = None,
                )
                if 'typename' in m['return_type']:
                    f['returns'] = m['return_type']['typename']['segments'][0]['name']
                for p in m['parameters']:
                    name = p['name']
                    if 'typename' in p['type']:
                        typ = p['type']['typename']['segments'][0]['name']
                        f['params'].append(dict(name=name, type=typ, ref=False))
                    elif 'ref_to' in p['type']:
                        typ = p['type']['ref_to']['typename']['segments'][0]['name']
                        f['params'].append(dict(name=name, type=typ, ref=True))

                r['methods'].append(f)
    return r


ds = parse('Delay')
# from IPython import embed; embed()
pprint(process(ds))
# ds = ds[22]

with open('parsed.yml', 'w') as f:
    yml = yaml.dump(ds)
    f.write(yml)
