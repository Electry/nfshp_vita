/*
 * gl_patch.c -- wrapper for PVR_PSP2 compiled with hardfp
 *
 * Copyright (C) 2021 Electry <dev@electry.sk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <PVR_PSP2/GLES/gl.h>
#include <PVR_PSP2/GLES/glext.h>

#include "gl_patch.h"

__attribute__((naked)) void glAlphaFunc_wrapper(GLenum func, GLclampf ref) {
  asm volatile (
    "vmov s0, r1\n"
    "b glAlphaFunc\n"
	);
}

__attribute__((naked)) void glClearColor_wrapper(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "vmov s3, r3\n"
    "b glClearColor\n"
	);
}

__attribute__((naked)) void glClearDepthf_wrapper(GLclampf depth) {
  asm volatile (
    "vmov s0, r0\n"
    "b glClearDepthf\n"
	);
}

__attribute__((naked)) void glClipPlanef_wrapper(GLenum plane, const GLfloat *equation) {
  asm volatile (
    "vmov s0, r1\n"
    "b glClipPlanef\n"
	);
}

__attribute__((naked)) void glColor4f_wrapper(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "vmov s3, r3\n"
    "b glColor4f\n"
	);
}

__attribute__((naked)) void glDepthRangef_wrapper(GLclampf zNear, GLclampf zFar) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "b glDepthRangef\n"
	);
}

__attribute__((naked)) void glDrawTexfOES_wrapper(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "vmov s3, r3\n"
    "b glDrawTexfOES\n"
	);
}

__attribute__((naked)) void glFogf_wrapper(GLenum pname, GLfloat param) {
  asm volatile (
    "vmov s0, r1\n"
    "b glFogf\n"
	);
}

__attribute__((naked)) void glFrustumf_wrapper(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "vmov s3, r3\n"
    "b glFrustumf\n"
	);
}

__attribute__((naked)) void glLightModelf_wrapper(GLenum pname, GLfloat param) {
  asm volatile (
    "vmov s0, r1\n"
    "b glLightModelf\n"
	);
}

__attribute__((naked)) void glLightf_wrapper(GLenum light, GLenum pname, GLfloat param) {
  asm volatile (
    "vmov s0, r2\n"
    "b glLightf\n"
	);
}

__attribute__((naked)) void glLineWidth_wrapper(GLfloat width) {
  asm volatile (
    "vmov s0, r0\n"
    "b glLineWidth\n"
	);
}

__attribute__((naked)) void glMaterialf_wrapper(GLenum face, GLenum pname, GLfloat param) {
  asm volatile (
    "vmov s0, r2\n"
    "b glMaterialf\n"
	);
}

__attribute__((naked)) void glMultiTexCoord4f_wrapper(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
  asm volatile (
    "vmov s0, r1\n"
    "vmov s1, r2\n"
    "vmov s2, r3\n"
    "b glMultiTexCoord4f\n"
	);
}

__attribute__((naked)) void glNormal3f_wrapper(GLfloat nx, GLfloat ny, GLfloat nz) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "b glNormal3f\n"
	);
}

__attribute__((naked)) void glOrthof_wrapper(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "vmov s3, r3\n"
    "b glOrthof\n"
	);
}

__attribute__((naked)) void glPointParameterf_wrapper(GLenum pname, GLfloat param) {
  asm volatile (
    "vmov s0, r1\n"
    "b glPointParameterf\n"
	);
}

__attribute__((naked)) void glPolygonOffset_wrapper(GLfloat factor, GLfloat units) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "b glPolygonOffset\n"
	);
}

__attribute__((naked)) void glRotatef_wrapper(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "vmov s3, r3\n"
    "b glRotatef\n"
	);
}

__attribute__((naked)) void glSampleCoverage_wrapper(GLclampf value, GLboolean invert) {
  asm volatile (
    "vmov s0, r0\n"
    "b glSampleCoverage\n"
	);
}

__attribute__((naked)) void glScalef_wrapper(GLfloat x, GLfloat y, GLfloat z) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "b glScalef\n"
	);
}

__attribute__((naked)) void glTexEnvf_wrapper(GLenum target, GLenum pname, GLfloat param) {
  asm volatile (
    "vmov s0, r2\n"
    "b glTexEnvf\n"
	);
}

__attribute__((naked)) void glTexGenfOES_wrapper(GLenum coord, GLenum pname, GLfloat param) {
  asm volatile (
    "vmov s0, r2\n"
    "b glTexGenfOES\n"
	);
}

__attribute__((naked)) void glTexParameterf_wrapper(GLenum target, GLenum pname, GLfloat param) {
  asm volatile (
    "vmov s0, r2\n"
    "b glTexParameterf\n"
	);
}

__attribute__((naked)) void glTranslatef_wrapper(GLfloat x, GLfloat y, GLfloat z) {
  asm volatile (
    "vmov s0, r0\n"
    "vmov s1, r1\n"
    "vmov s2, r2\n"
    "b glTranslatef\n"
	);
}
