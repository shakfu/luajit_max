#!/usr/bin/env python3

import os
from pathlib import Path
import yaml
from pprint import pprint
import dataclasses
import json

import cxxheaderparser
from cxxheaderparser.simple import parse_file


STK = Path('build/deps/stk-install')
STK_INCLUDE = STK / 'include/stk'


class Param:
    def __init__(self, name, type, is_ref=False):
        self.name = name
        self.type = type
        self.is_ref = is_ref

    def __str__(self):
        suffix = "&" if self.is_ref else ""
        return f"{self.type}{suffix} {self.name}"

class MethodParam(Param):
    def __str__(self):
        suffix = "&" if self.is_ref else ""
        return f"{self.type}{suffix}"


class Method:
    def __init__(self, name, params=None, returns=None, parent=None):
        self.name = name
        self.params = params or []
        self.returns = returns
        self.parent = parent

    def __str__(self):
        name = self.name
        klass = self.parent.name
        if self.name == 'tick':
            def get_type(p):
                suffix = "&" if p.is_ref else ""
                prefix = "stk::" if p.type.startswith('Stk') else ""
                return f"{prefix}{p.type}{suffix}"

            params = ', '.join(get_type(p) for p in self.params)
            return f'        luabridge::overload<{params}>(&stk::{klass}::{name}),'
        return f'    .addFunction("{name}", &stk::{klass}::{name})'

    def add_param(self, name, type, is_ref=False):
        self.params.append(Param(name, type, is_ref))


class Constructor(Method):
    def __init__(self, name, params=None, parent=None):
        self.name = name
        self.params = params or []
        self.parent = parent

    def __str__(self):
        if self.params:
            params = ", ".join(str(p) for p in self.params)
            return f'    .addConstructor<void (*) ({params})>()'
        else:
            return f'    .addConstructor<void ()> ()'


class CppClass:
    def __init__(self, name, destructor=None, constructors=None, methods=None):
        self.name = name
        self.destructor = destructor
        self.constructors = constructors or []
        self.methods = methods or []

    def __str__(self):
        name = self.name
        start = f'.beginClass <stk::{name}> ("{name}")'
        end = '.endClass()'
        space = ' '*4
        res = [start]
        for c in self.constructors:
            res.append(str(c))
        for m in self.methods:
            res.append(str(m))
        res.append(end)

        return "\n".join(res)






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

def get_class(d):
    d = d['namespace']['namespaces']['stk']

    klass = CppClass(name=d['classes'][0]['class_decl']['typename']['segments'][0]['name'])

    for m in d['classes'][0]['methods']:
        if m['access'] == 'public':
            if m['constructor']:
                c = Constructor(name=m['name']['segments'][0]['name'], parent=klass)

                for p in m['parameters']:
                    name = p['name']
                    if 'typename' in p['type']:
                        typ = p['type']['typename']['segments'][0]['name']
                        c.params.append(Param(name=name, type=typ, is_ref=False))
                    elif 'ref_to' in p['type']:
                        typ = p['type']['ref_to']['segments'][0]['name']
                        c.params.append(Param(name=name, type=typ, is_ref=True))

                klass.constructors.append(c)
            elif m['destructor']:
                klass.destructor = m['name']['segments'][0]['name']
            else:
                f = Method(name = m['name']['segments'][0]['name'], parent=klass)

                if 'typename' in m['return_type']:
                    f.returns = m['return_type']['typename']['segments'][0]['name']
                for p in m['parameters']:
                    name = p['name']
                    if 'typename' in p['type']:
                        typ = p['type']['typename']['segments'][0]['name']
                        f.params.append(MethodParam(name=name, type=typ, is_ref=False))
                    elif 'ref_to' in p['type']:
                        typ = p['type']['ref_to']['typename']['segments'][0]['name']
                        f.params.append(MethodParam(name=name, type=typ, is_ref=True))

                klass.methods.append(f)

    return klass


def main():
    classes = []
    for f in STK_INCLUDE.iterdir():
        name = f.stem         
        try:
            obj = parse_file(f)
            d = dataclasses.asdict(obj)
            klass = get_class(d)
        except cxxheaderparser.errors.CxxParseError:
            print("ERROR: ", name)
            continue
        except KeyError:
            print("KeyError: ", name)
            continue
        except IndexError:
            print("IndexError: ", name)
            continue

        classes.append(klass)

    for c in classes:
        print(c)

    # with open('parsed.yml', 'w') as f:
    #     yml = yaml.dump(ds)
    #     f.write(yml)

main()
