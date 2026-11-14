#!/usr/bin/env python3
"""Bundle release media in a custom indexed binary container. No loose artwork/audio."""
import argparse
import pathlib
import struct
import tempfile
import os

MASK=(1<<64)-1
KEY=0x9D4F2DC37AB610E5

def hash32(data):
    h=2166136261
    for b in data:
        h=((h^b)*16777619)&0xffffffff
    return h

def seed(path):
    h=14695981039346656037
    for b in path:
        h=((h^b)*1099511628211)&MASK
    return h^KEY

def scramble(data,path):
    state=seed(path)
    output=bytearray(data)
    for i in range(len(output)):
        if i%8==0:
            state=(state+0x9E3779B97F4A7C15)&MASK
            z=state
            z=((z^(z>>30))*0xBF58476D1CE4E5B9)&MASK
            z=((z^(z>>27))*0x94D049BB133111EB)&MASK
            word=z^(z>>31)
        output[i]^=(word>>(8*(i%8)))&255
    return output

def pack(source,target):
    media=sorted(p for p in source.rglob('*') if p.is_file() and
                 'licenses' not in p.relative_to(source).parts and p.suffix!='.vpak')
    if not media:
        raise SystemExit('Refusing to build an empty asset pack')
    target.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=target.parent,delete=False) as temp:
        temp.write(b'VLPAK007')
        temp.write(struct.pack('<I',len(media)))
        for file in media:
            name=file.relative_to(source).as_posix().encode('utf-8')
            data=file.read_bytes()
            if len(name)>255 or len(data)>64*1024*1024:
                raise ValueError('Asset too large: '+str(file))
            temp.write(struct.pack('<H',len(name)))
            temp.write(name)
            temp.write(struct.pack('<II',len(data),hash32(data)))
            temp.write(scramble(data,name))
        temporary=temp.name
    os.replace(temporary,target)
    os.chmod(target,0o644)
    print(f'Packed {len(media)} resources into {target}: {target.stat().st_size:,} bytes')

if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('source',type=pathlib.Path)
    parser.add_argument('target',type=pathlib.Path)
    args=parser.parse_args()
    pack(args.source,args.target)
