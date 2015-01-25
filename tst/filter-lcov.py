#!/usr/bin/python
import re

def remove_user(dst_file_name, src_file_name):
    inf = open(src_file_name, 'r')
    out = open(dst_file_name, 'w')
    output = True
    inline = inf.readline()
    while inline:
        if re.match('SF:/usr/lib/gcc/', inline):
            output = False;
        if output:
            out.write(inline)
        if re.match('end_of_record', inline):
            output = True;
        inline = inf.readline()
    inf.close()
    out.close()
if __name__ == '__main__':
    import sys
    remove_user(sys.argv[1], sys.argv[2])
