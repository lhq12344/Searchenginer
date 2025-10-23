#!/usr/bin/env python3
import struct
import math
from statistics import mean

path='/home/lihaoqian/myproject/SearchEngine_v2/Make_Page/page/vectors.dat'
with open(path,'rb') as f:
    magic=f.read(4)
    if magic!=b'VEC1':
        print('bad magic',magic)
        raise SystemExit(1)
    dim,=struct.unpack('i',f.read(4))
    nvec,=struct.unpack('i',f.read(4))
    print('dim=',dim,'nvec=',nvec)
    docs=[]
    vecs=[]
    for i in range(nvec):
        docid,=struct.unpack('i',f.read(4))
        data=f.read(4*dim)
        vals=struct.unpack('%df'%dim,data)
        docs.append(docid)
        vecs.append(vals)

# compute norms and similarities
norms=[math.sqrt(sum(x*x for x in v)) for v in vecs]
print('norms: min={:.6f} max={:.6f} mean={:.6f}'.format(min(norms),max(norms),mean(norms)))

# similarities with first vector
ref=vecs[0]
sims=[]
for v in vecs:
    dot=sum(a*b for a,b in zip(ref,v))
    sims.append(dot)

print('similarity to first: min={:.6f} max={:.6f} mean={:.6f}'.format(min(sims),max(sims),mean(sims)))
print('first 30 similarities:')
for i,s in enumerate(sims[:30]):
    print(i,docs[i],'{:.6f}'.format(s))

# check if many are nearly identical to first
count_very_close=sum(1 for s in sims if s>0.999)
print('count with sim>0.999=',count_very_close)

# variance per dimension across first 100 vectors
import statistics
m= min(100,len(vecs))
vars=[statistics.pvariance([vecs[i][d] for i in range(m)]) for d in range(dim)]
print('variance stats over first {} vectors: min={:.6g} max={:.6g} mean={:.6g}'.format(m,min(vars),max(vars),mean(vars)))

# check unique vectors (hash)
hashes=set()
for v in vecs:
    hashes.add(tuple(round(x,6) for x in v))
print('unique vectors (rounded 1e-6):',len(hashes))
