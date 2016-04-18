#!/bin/python

from waflib import *
import os, sys

top = '.'
out = 'build'

projname = 'progeny'
coreprog_name = projname

g_cflags = ["-Wall", "-Wextra", "-std=c11"]
def btype_cflags(ctx):
	return {
		"DEBUG"   : g_cflags + ["-Og", "-ggdb3", "-march=core2", "-mtune=native"],
		"NATIVE"  : g_cflags + ["-Ofast", "-march=native", "-mtune=native"],
		"RELEASE" : g_cflags + ["-O3", "-march=core2", "-mtune=generic"],
	}.get(ctx.env.BUILD_TYPE, g_cflags)

def options(opt):
	opt.load("gcc")
	opt.add_option('--build_type', dest='build_type', type="string", default='RELEASE', action='store', help="DEBUG, NATIVE, RELEASE")

def configure(ctx):
	ctx.load("gcc")
	ctx.check(features='c cprogram', lib='dl', uselib_store='DL')
	ctx.check(features='c cprogram', lib='xcb', uselib_store='XCB')
	ctx.check(features='c cprogram', lib='jemalloc', uselib_store='JMAL')
	btup = ctx.options.build_type.upper()
	if btup in ["DEBUG", "NATIVE", "RELEASE"]:
		Logs.pprint("PINK", "Setting up environment for known build type: " + btup)
		ctx.env.BUILD_TYPE = btup
		ctx.env.CFLAGS = btype_cflags(ctx)
		Logs.pprint("PINK", "CFLAGS: " + ' '.join(ctx.env.CFLAGS))
		if btup == "DEBUG":
			ctx.define("PROGENY_DEBUG", 1)
	else:
		Logs.error("UNKNOWN BUILD TYPE: " + btup)

def build(bld):
	files =  bld.path.ant_glob('src/*.c')
	coreprog = bld (
		features = "c cprogram",
		target = coreprog_name,
		source = files,
#		lib = ["pthread"],
		uselib = ['XCB', 'DL', 'JMAL']
	)
	
def clean(ctx):
	pass
