from waflib.TaskGen import extension

import sys

blddir="build"
srcdir="."

def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    opt.load('waf_unit_test')

def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx waf_unit_test')
    conf.env['CXXFLAGS'] = conf.env['CFLAGS'] = ['-g', '-O2', '-Wall']
    if sys.platform == 'darwin':
        pass
    else:
        conf.env['DEFINES'] += ['SVR4','_GNU_SOURCE']
        conf.env['CFLAGS'] += ['-fomit-frame-pointer']
    conf.check_cc(lib='termcap', uselib_store='TERM')

def build(bld):
    bld.recurse('extra')
    bld.recurse('src')
    bld.recurse('tests')
