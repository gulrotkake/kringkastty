from waflib.TaskGen import extension
from waflib.Task import Task

import sys, subprocess

blddir="build"
srcdir="."

def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')

class content(Task):
    def run(self):
        template = self.inputs[0].read()
        for p in self.inputs[1:]:
            template = template.replace('[[%s]]'%p.name, p.read())
        xxd=subprocess.Popen(['xxd', '-i'], stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE)
        try:
            out, err = xxd.communicate(input=template)
            self.outputs[0].write('unsigned char content_h[] = {\n%s\n};\n'%out+
                                  'unsigned int content_h_len = %d;\n'%(out.count(',')+1))
            return 0
        except Exception as e:
            print e
            return 1

def configure(conf):
    conf.load('compiler_c')
    conf.load('compiler_cxx')
    conf.env['CXXFLAGS'] = conf.env['CFLAGS'] = ['-g', '-O2', '-Wall']
    if sys.platform == 'darwin':
        conf.env.INCLUDES += ['/usr/include/machine']
        conf.check(header_name='endian.h', features='c cprogram')
    else:
        conf.env['DEFINES'] += ['SVR4','_GNU_SOURCE']
        conf.env['CFLAGS'] += ['-fomit-frame-pointer']
    conf.check_cc(lib='termcap', uselib_store='TERM')

def build(bld):
    content_h = content(env=bld.env)
    content_h.set_inputs([
        bld.path.find_resource('extra/html/content.tmpl'),
        bld.path.find_resource('extra/js/main.js'),
        bld.path.find_resource('extra/js/term.js'),
        bld.path.find_resource('extra/css/content.css')
    ])
    content_h.set_outputs([bld.path.find_or_declare('include/content.h')])
    bld.add_to_group(content_h)
    bld.recurse('src')
