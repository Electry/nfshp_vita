#ifndef __GL_PATCH_H__
#define __GL_PATCH_H__

void glAlphaFunc_wrapper(GLenum func, GLclampf ref);
void glClearColor_wrapper(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void glClearDepthf_wrapper(GLclampf depth);
void glClipPlanef_wrapper(GLenum plane, const GLfloat *equation);
void glColor4f_wrapper(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void glDepthRangef_wrapper(GLclampf zNear, GLclampf zFar);
void glDrawTexfOES_wrapper(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);
void glFogf_wrapper(GLenum pname, GLfloat param);
void glFrustumf_wrapper(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void glLightModelf_wrapper(GLenum pname, GLfloat param);
void glLightf_wrapper(GLenum light, GLenum pname, GLfloat param);
void glLineWidth_wrapper(GLfloat width);
void glMaterialf_wrapper(GLenum face, GLenum pname, GLfloat param);
void glMultiTexCoord4f_wrapper(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void glNormal3f_wrapper(GLfloat nx, GLfloat ny, GLfloat nz);
void glOrthof_wrapper(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar);
void glPointParameterf_wrapper(GLenum pname, GLfloat param);
void glPointSize_wrapper(GLfloat size);
void glPolygonOffset_wrapper(GLfloat factor, GLfloat units);
void glRotatef_wrapper(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void glSampleCoverage_wrapper(GLclampf value, GLboolean invert);
void glScalef_wrapper(GLfloat x, GLfloat y, GLfloat z);
void glTexEnvf_wrapper(GLenum target, GLenum pname, GLfloat param);
void glTexGenfOES_wrapper(GLenum coord, GLenum pname, GLfloat param);
void glTexParameterf_wrapper(GLenum target, GLenum pname, GLfloat param);
void glTranslatef_wrapper(GLfloat x, GLfloat y, GLfloat z);

#endif
