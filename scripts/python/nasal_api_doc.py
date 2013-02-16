#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2012 Adrian Musceac
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import os, sys, glob
import io
import re, string

"""Script which generates an API documentation file for Nasal libraries
located inside $FGROOT/Nasal/
Usage: nasal_api_doc.py parse [path to $FGROOT/Nasal/]
Or configure the local path below, and ommit the path in the console.
The API doc in HTML format is generated in the current working directory"""

########### Local $FGROOT/Nasal/ path ##########
NASAL_PATH="../fgfs/fgdata/Nasal/"


def get_files(nasal_dir):
	if nasal_dir[-1]!='/':
		nasal_dir+='/'
	try:
		os.stat(nasal_dir)
	except:
		print "The path does not exist"
		sys.exit()
	fgroot_dir = nasal_dir.rstrip('/').replace('Nasal','')
	f_version = open(fgroot_dir+'version','rb')
	version = f_version.read(256).rstrip('\n')
	top_level = []
	modules = []
	top_namespaces = []
	files_list = os.listdir(nasal_dir)
	for f in files_list:
		if f.find(".nas")!=-1:
			top_level.append(f)
			continue
		if os.path.isdir(nasal_dir + f):
			modules.append(f)
	top_level.sort()
	modules.sort()
	if len(top_level) ==0:
		print "This does not look like the correct $FGROOT/Nasal path"
		sys.exit()
	if len(modules)==0:
		print "Warning: could not find any submodules"
	for f in top_level:
		namespace=f.replace(".nas","")
		functions=parse_file(nasal_dir + f)
		top_namespaces.append([namespace,functions])
	for m in modules:
		files=glob.glob(nasal_dir+m+"/*.nas")
		for f in files:
			functions=parse_file(f)
			top_namespaces.append([m,functions])

	output_text(top_namespaces,modules,version)


def output_text(top_namespaces,modules,version):
	fw=open('./nasal_api_doc.html','wb')
	buf='<html><head>\
	<title>Nasal API</title>\
	<style>\n\
	a.main_module_link {margin-left:30px;display:block;float:left;}\
	div.container {background-color:#eee;clear:left;margin-top:20px;}\
	h2.namespace_title {padding-left:20px;color:#fff;background-color:#8888AC}\
	h4.class_function {padding-left:20px;background-color:#eee;color:#000033}\
	h4.class_definition {padding-left:20px;background-color:#eee;color:#000033}\
	h4.function {padding-left:20px;background-color:#eee;color:#000033}\
	hr {margin-left:30px;margin-right:30px;}\
	div.comments {padding-left:40px;display:inline;font-size:12px;}\
	</style>\n\
	</head><body style="width:1024px;">'

	buf+='<h1 style="padding-left:20px;display:block;color:#fff;background-color:#555588;">\
	Nasal $FGROOT Library<br/><span style="font-size:12px;">Flightgear version: '+version+'\
	<br/>This file is generated automatically by scripts/python/nasal_api_doc.py\
	</span></h1>\
	<br/><a href="http://plausible.org/nasal">Nasal documentation</a>&nbsp;&nbsp;\
	<a href="http://wiki.flightgear.org/Nasal_scripting_language">Flightgear Nasal documentation</a>\n<div style="float:right;">&nbsp;'
	buf+='<h2 style="font-size:14px;height:450px;width:250px;overflow:scroll;display:block;position:fixed;top:20px;right:20px;background-color:#8888AC;border:1px solid black;">\n'
	done=[]
	for namespace in top_namespaces:
		color='0000cc'
		if namespace[0] in modules:
			color='cc0000'
		if namespace[0] not in done:
			buf+='<a class="main_module_link" style="color:'+color+'" href="#'+namespace[0]+'">'+namespace[0]+'</a>&nbsp;<br/>\n'
			done.append(namespace[0])
	buf+='</h2></div>\n'
	done2=[]
	for namespace in top_namespaces:
		if namespace[0] not in done2:
			buf+='<div class="container" style="">\n'
			buf += '<h2 class="namespace_title"><a name="'+namespace[0]+'">'+namespace[0]+'</a></h2>\n'
			done2.append(namespace[0])
		for functions in namespace[1]:
			class_func=functions[0].split('.')
			if len(class_func)>1:
				f_name=''
				if class_func[1].find('_')==0:
					f_name='<font color="#0000cc">'+class_func[1]+'</font>'
				else:
					f_name=class_func[1]
				if class_func[1]!='':
					buf+= '<div><h4 class="class_function"><b>'\
						+namespace[0]+'</b>'+ "." + '<b><i>'+class_func[0]+'</i></b>'+'<b>.'+f_name+'</b>'+' ( <font color="#cc0000">'+ functions[1]+ '</font> )' +'</h4>\n'
				else:
					buf+= '<div><h4 class="class_definition"><b>'\
						+namespace[0]+'</b>'+ "." + '<b><i><u><font color="#000000">'+class_func[0]+'</font></u></i></b>' +'</h4>\n'
			else:
				if functions[0].find('_')==0:
					f_name='<font color="#0000cc">'+functions[0]+'</font>'
				else:
					f_name=functions[0]
				buf+= '<div><h4 class="function"><b>'\
					+namespace[0]+'</b>'+ "." + '<b>'+f_name+'</b>'+ ' ( <font color="#cc0000">'+ functions[1]+ '</font> )' +'</h4>\n'
			for comment in functions[2]:
				if comment.find('=====')!=-1:
					buf+='<hr/>'
				else:
					tempComment = comment.replace('#','').replace('<','&lt;').replace('>','&gt;')
					if string.strip(tempComment)!="":
						buf+= '<div class="comments">'+tempComment+'</div><br/>\n'
			buf+='</div>\n'
		if namespace[0] not in done2:
			buf+='</div>\n'
	buf+='</body></html>'
	fw.write(buf)
	fw.close()

def parse_file(filename):
	fr=open(filename,'rb')
	content=fr.readlines()
	i=0
	retval=[]
	classname=""
	for line in content:
		match=re.search('^var\s+([A-Za-z0-9_-]+)\s*=\s*func\s*\(?([A-Za-z0-9_\s,=.\n-]*)\)?',line)
		if match!=None:
			func_name=match.group(1)
			comments=[]
			param=match.group(2)
			if(line.find(')')==-1 and line.find('(')!=-1):
				k=i+1
				while(content[k].find(')')==-1):
					param+=content[k].rstrip('\n')
					k+=1
				param+=content[k].split(')')[0]
			j=i-1
			count=0
			while ( j>i-35 and j>-1):
				if count>3:
					break
				if len(content[j])<2:
					j-=1
					count+=1
					continue
				if re.search('^\s*#',content[j])!=None:
					comments.append(content[j].rstrip('\n'))
					j-=1
				else:
					break
			if(len(comments)>1):
				comments.reverse()
			retval.append((func_name, param,comments))
			i+=1
			continue

		match3=re.search('^var\s*([A-Za-z0-9_-]+)\s*=\s*{\s*(\n|})',line)
		if match3!=None:
			classname=match3.group(1)

			comments=[]

			j=i-1
			count=0
			while ( j>i-35 and j>-1):
				if count>3:
					break
				if len(content[j])<2:
					j-=1
					count+=1
					continue
				if re.search('^\s*#',content[j])!=None:
					comments.append(content[j].rstrip('\n'))
					j-=1
				else:
					break
			if(len(comments)>1):
				comments.reverse()
			retval.append((classname+'.', '',comments))
			i+=1
			continue

		match2=re.search('^\s*([A-Za-z0-9_-]+)\s*:\s*func\s*\(?([A-Za-z0-9_\s,=.\n-]*)\)?',line)
		if match2!=None:
			func_name=match2.group(1)
			comments=[]
			param=match2.group(2)
			if(line.find(')')==-1 and line.find('(')!=-1):
				k=i+1
				while(content[k].find(')')==-1):
					param+=content[k].rstrip('\n')
					k+=1
				param+=content[k].split(')')[0]
			j=i-1
			count=0
			while ( j>i-35 and j>-1):
				if count>3:
					break
				if len(content[j])<2:
					j-=1
					count+=1
					continue
				if re.search('^\s*#',content[j])!=None:
					comments.append(content[j].rstrip('\n'))
					j-=1
				else:
					break
			if(len(comments)>1):
				comments.reverse()
			if classname =='':
				continue
			retval.append((classname+'.'+func_name, param,comments))
			i+=1
			continue

		match4=re.search('^([A-Za-z0-9_-]+)\.([A-Za-z0-9_-]+)\s*=\s*func\s*\(?([A-Za-z0-9_\s,=\n.-]*)\)?',line)
		if match4!=None:
			classname=match4.group(1)
			func_name=match4.group(2)
			comments=[]
			param=match4.group(3)
			if(line.find(')')==-1 and line.find('(')!=-1):
				k=i+1
				while(content[k].find(')')==-1):
					param+=content[k].rstrip('\n')
					k+=1
				param+=content[k].split(')')[0]
			j=i-1
			count=0
			while ( j>i-35 and j>-1):
				if count>3:
					break
				if len(content[j])<2:
					j-=1
					count+=1
					continue
				if re.search('^\s*#',content[j])!=None:
					comments.append(content[j].rstrip('\n'))
					j-=1
				else:
					break
			if(len(comments)>1):
				comments.reverse()
			retval.append((classname+'.'+func_name, param,comments))
			i+=1
			continue

		i+=1
	return retval



if __name__ == "__main__":
	if len(sys.argv) <2:
		print 'Usage: nasal_api_doc.py parse [path to $FGROOT/Nasal/]'
		sys.exit()
	else:
		if sys.argv[1]=='parse':
			if len(sys.argv) <3:
				nasal_path=NASAL_PATH
			else:
				nasal_path=sys.argv[2]
			get_files(nasal_path)
		else:
			print 'Usage: nasal_api_doc.py parse [path to $FGROOT/Nasal/]'
			sys.exit()
