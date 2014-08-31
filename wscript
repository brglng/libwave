#!python
# encoding: utf-8

APPNAME = 'libwave'
VERSION = '1.0.0'

top = '.'
out = '_build'

subdirs = """
    src
    app/diff-wave
""".split()

def options(opt):
    [opt.recurse(s) for s in subdirs]

def configure(conf):
    [conf.recurse(s) for s in subdirs]

def build(bld):
    [bld.recurse(s) for s in subdirs]

def dist(ctx):
    ctx.algo = 'zip'
    ctx.files = ctx.path.ant_glob('waf **/wscript **/*.c **/*.h')
