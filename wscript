#!/bin/python

from waflib import *
import os, sys, traceback, subprocess, binascii

top = '.'
out = 'build'
shad = 'shad'

projname = 'progeny'
coreprog_name = projname

g_cflags = ["-Wall", "-Wextra", "-std=c++17"]
def btype_cflags(ctx):
	return {
		"DEBUG"   : g_cflags + ["-Og", "-ggdb3", "-march=core2", "-mtune=native"],
		"NATIVE"  : g_cflags + ["-Ofast", "-march=native", "-mtune=native"],
		"RELEASE" : g_cflags + ["-O3", "-march=core2", "-mtune=generic"],
	}.get(ctx.env.BUILD_TYPE, g_cflags)

def options(opt):
	opt.load("g++")
	opt.add_option('--build_type', dest='build_type', type="string", default='RELEASE', action='store', help="DEBUG, NATIVE, RELEASE")

def configure(ctx):
	ctx.load("g++")
	ctx.check(features='c cprogram', lib='dl', uselib_store='DL')
	ctx.check(features='c cprogram', lib='xcb', uselib_store='XCB')
	ctx.check(features='c cprogram', lib='jemalloc', uselib_store='JMAL')
	btup = ctx.options.build_type.upper()
	if btup in ["DEBUG", "NATIVE", "RELEASE"]:
		Logs.pprint("PINK", "Setting up environment for known build type: " + btup)
		ctx.env.BUILD_TYPE = btup
		ctx.env.CXXFLAGS = btype_cflags(ctx)
		Logs.pprint("PINK", "CXXFLAGS: " + ' '.join(ctx.env.CXXFLAGS))
		if btup == "DEBUG":
			ctx.define("PROGENY_DEBUG", 1)
	else:
		Logs.error("UNKNOWN BUILD TYPE: " + btup)
		
def shaders(ctx):
	
	ret = subprocess.call(['mkdir','-p',os.path.join(top, out, shad)])
	
	if ret != 0:
		raise ctx.errors.WafError("shaders: could not create output directory")
	
	df = open(os.path.join(top, out, shad, 'shaders.cpp'), 'wb')
	df.write("#include \"vk.hpp\"\n\n".encode())
	
	for f in os.listdir(os.path.join(top, shad)):
		fs = f.split('.')
		proc = subprocess.Popen(['glslangValidator','-V',os.path.join(top, shad, f),'-o',os.path.join(top, out, shad, 'temp.spv')], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		pout, perr = proc.communicate()
		if proc.returncode != 0:
			raise ctx.errors.WafError("shaders: glsl shader failed to compile: \nstdout: " + pout.decode("utf-8") + "\n\nstderr: " + perr.decode("utf-8"))
		spvf = open(os.path.join(top, out, shad, 'temp.spv'), 'rb')
		
		data = spvf.read()
		datas = list(data)
		dataf = ""
		for b in datas:
			dataf += ("0x{0:02x}, ".format(b))
		
		df.write("uint8_t const {0} [] = {{{1}}};\n".format("vk::spirv::{0}_{1}".format(fs[1], fs[0]), dataf).encode())
		df.write("size_t {0}_size = {1};\n".format("vk::spirv::{0}_{1}".format(fs[1], fs[0]), len(data)).encode())
		Logs.pprint("YELLOW", "shader \"{0}\" compiled successfully to SPIR-V and hardcoded to shaders.cpp ({1}, {2})".format(f, "vk::spirv::{0}_{1}".format(fs[1], fs[0]), "vk::spirv::{0}_{1}_size".format(fs[1], fs[0])))
		
def build(bld):
	files =  bld.path.ant_glob('src/*.cpp')
	files += [os.path.join(top, out, shad, 'shaders.cpp')]
	
	coreprog = bld (
		features = "cxx cxxprogram",
		target = coreprog_name,
		source = files,
		uselib = ['XCB', 'DL', 'JMAL'],
		includes = os.path.join(top, 'src'),
	)
	
def clean(ctx):
	pass
