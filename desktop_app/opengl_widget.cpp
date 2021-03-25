/** @file
    @brief Header

    @date 2015

    @author
    Russell Taylor
    <russ@reliasolve.com>
    <http://sensics.com>
*/

// Copyright 2015 Sensics, Inc.
//
// (Licensed under the Apache License, Version 2.0)
// SPDX-License-Identifier: Apache-2.0

#include "opengl_widget.h"
#include "mainwindow.h"

#include <QtGui>
#include <QtOpenGL>
#include <QColor>
#include <QFileDialog>
#include <QCursor>
#include <QMainWindow>
#include <sstream>
#include <math.h>
#include <stdio.h>
#include <quat.h>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

//----------------------------------------------------------------------
// File-scoped globals

// Minimum interval between analog and tracker trigger events.
static const double ANALOG_DELAY = 0.5;
static const double TRACKER_DELAY = 0.5;

//----------------------------------------------------------------------
// Helper functions

//----------------------------------------------------------------------
// Member functions

OpenGL_Widget::OpenGL_Widget(QWidget *parent)
    : QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    d_last_frame_time = 0;
    d_frame_count = 0;

    // Initial target radius is 5 pixels.
    d_r = 12;

    // Initial clear color is black;
    d_clearColor = Qt::black;

    // Start out oscillating the cursor and display
    d_oscillate = true;
    d_oscillate_freq = 2;
    d_oscillate_phase = 0;
    d_oscillate_mag = 60;

    // Call the timer that will cause OpenGL to draw
    // faster than the maximum display update rate.
    connect(&d_timer,SIGNAL(timeout()),this,SLOT(idle()));
    d_osc_time.start();
    d_timer.start();

    // Set a cursor that is symmetric around its center.
    this->setCursor(Qt::CrossCursor);

    // Initialize VRPN objects and other things set by slots.
    d_button = NULL;
    d_analog = NULL;
    d_tracker = NULL;

    // Initialize the vertex array based on the number of
    // quads;
    d_num_quads = 1;
    d_vertex_array = NULL;
    updateVertexArray();

    // Set the minimum and maximum rotations such that they
    // will be overridden by any valid readings.
    d_tracker_min_rotation = 1e10;
    d_tracker_max_rotation = -1e10;
}

OpenGL_Widget::~OpenGL_Widget()
{
    if (d_button != NULL) { delete d_button; }
    if (d_analog != NULL) { delete d_analog; }
    if (d_tracker != NULL) { delete d_tracker; }
}

void OpenGL_Widget::useVRPNButton(QString name, int which)
{
	d_button = new vrpn_Button_Remote(name.toStdString().c_str());
	d_button->register_change_handler(this, handleButton);
	d_button_to_use = which;
}

void OpenGL_Widget::useVRPNAnalog(QString name, int which, double threshold)
{
    d_analog = new vrpn_Analog_Remote(name.toStdString().c_str());
    d_analog->register_change_handler(this, handleAnalog);
    d_analog_to_use = which;
    d_analog_threshold = threshold;
    d_analog_last_trigger.tv_sec = 0;
    d_analog_last_trigger.tv_sec = 0;
}

void OpenGL_Widget::useVRPNTracker(QString name, int sensor, double transThresh, double rotThresh)
{
    d_tracker = new vrpn_Tracker_Remote(name.toStdString().c_str());
    d_tracker->register_change_handler(this, handleTracker, sensor);
    d_tracker_trans_thresh = transThresh;
    d_tracker_rot_thresh = rotThresh;
    d_tracker_rotate_axis = -1; // Disable rotation.
    d_tracker_last_trigger.tv_sec = 0;
    d_tracker_last_trigger.tv_sec = 0;
}

void OpenGL_Widget::useVRPNTrackerRotate(QString name, int sensor, int whichAxis)
{
    d_tracker = new vrpn_Tracker_Remote(name.toStdString().c_str());
    d_tracker->register_change_handler(this, handleTracker, sensor);
    d_tracker_rotate_axis = whichAxis;
}

void OpenGL_Widget::idle(void)
{
    if (d_button != NULL) { d_button->mainloop(); }
    if (d_analog != NULL) { d_analog->mainloop(); }
    if (d_tracker != NULL) { d_tracker->mainloop(); }
    updateGL();
}

void OpenGL_Widget::initializeGL()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    static GLfloat lightPosition[4] = { 0.5, 5.0, 7.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glDisable(GL_MULTISAMPLE);

    // Makes the colors for the primitives be what we want.
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
}

void OpenGL_Widget::paintGL()
{
    if (d_oscillate) {
        // Figure out the new location in the oscillation.  Move both
        // the cursor and the displayed box to their respective locations.
        // Call code to move the cursor here.  Put this code immediately
        // before the 3D rendering loop so there is not additional latency
        // due to intervening operations.
        double ms = d_osc_time.elapsed();
        double sec = ms / 1000;
        int x = d_width / 2 + d_oscillate_mag * cos(sec * (2* M_PI * d_oscillate_freq));
        int y = d_height / 2 + d_oscillate_mag * sin(sec * (2* M_PI * d_oscillate_freq));
        double phase_cycles = d_oscillate_phase / 360.0;
        int box_x = d_width / 2 + d_oscillate_mag *
                cos(2* M_PI * (sec * d_oscillate_freq + phase_cycles));
        int box_y = d_height / 2 + d_oscillate_mag *
                sin(2* M_PI * (sec * d_oscillate_freq + phase_cycles));
        QCursor::setPos(QWidget::mapToGlobal(QPoint(x,y)));
        d_x = box_x;
        d_y = d_height - box_y;    // Y inverted for OpenGL and mouse
    }

    qglClearColor(d_clearColor);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -10.0);

    // Translate and scale so that a quad from -1 to 1 in X and Y will
    // render correctly.  This lets us re-use our vertex array and not have
    // to change it every frame.
    glTranslatef(d_x, d_y, 0);
    glScalef(d_r, d_r, 0);

    // Draw a square box at the specified coordinates.
    // Overdraw this as many times as we are asked to draw quads.
    glColor3f(1,0,0);
    glEnableClientState( GL_VERTEX_ARRAY );
    glVertexPointer( 3, GL_FLOAT, 0, d_vertex_array );
    glDrawArrays( GL_TRIANGLES, 0, d_num_quads * 6 );
    glDisableClientState( GL_VERTEX_ARRAY );

    // Draw instructions on the screen.  This will provide additional rendering load,
    // but it did not change the results in our early testing.
    glColor3f(1,1,1);
    renderText(30, 30, "OSVR 2D vs. 3D rendering latency test program version 03.00.00 (run with -fullscreen to remove borders)");
    renderText(50, 50, "Press + to increase oscillation");
    renderText(50, 70, "Press - to decrease oscillation");
    renderText(50, 90, "Until you find the slowest oscillation where the cursor and square are in phase");

    renderText(50,110, "Press o to toggle oscillation");
    renderText(50,130, "Press ESC/q to quit");
    renderText(50,150, "Left mouse button drags square when not oscillating");
    renderText(50,170, "Middle mouse button teleports mouse when not oscillating");
    renderText(50,190, "Right mouse button toggles background brightness");

    double latency = (d_oscillate_phase/360) / (d_oscillate_freq);
    double latency_ms = latency * 1000;
    std::stringstream msg;
    msg << "Latency = " << latency_ms << "ms, Phase = " << d_oscillate_phase << " degrees";
    renderText(50,220, msg.str().c_str());

    if (++d_frame_count == 30) {
        double now = d_osc_time.elapsed() / 1000.0;
        double duration = now - d_last_frame_time;
        msg.str("");
        msg << "Rendering " << d_num_quads << " quads; ";
        msg << "Frame rate = " << d_frame_count/duration << " Hz";
        d_frame_msg = msg.str();
        d_last_frame_time = now;
        d_frame_count = 0;
    }
    renderText(50,240, d_frame_msg.c_str());

}

void OpenGL_Widget::resizeGL(int width, int height)
{
    d_width = width;
    d_height = height;
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Make the window one unit high (-0.5 to 0.5) and have an aspect ratio that matches
    // the aspect ratio of the window.  We also make the left side of the window be at
    // the origin.
    float aspect;
    if ((height <= 0) || (width < 0)) {
        aspect = 1.0;
    } else {
        aspect = static_cast<float>(width)/height;
    }
    glOrtho(0, d_width-1, 0, d_height-1, 5.0, 15.0);
    glMatrixMode(GL_MODELVIEW);
    d_x = d_width / 2;
    d_y = d_height / 2;    
}


void OpenGL_Widget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Q:
        //d_timer->stop();
        QApplication::quit();
        break;
    case Qt::Key_Plus:
        d_oscillate_phase += 2.5;
        break;
    case Qt::Key_Minus:
        d_oscillate_phase -= 2.5;
        break;
    case Qt::Key_O:
        d_oscillate = !d_oscillate;
        break;
    case Qt::Key_Left:
        break;
    case Qt::Key_Right:
        break;
    case Qt::Key_Down:
        break;
    case Qt::Key_Up:
        break;
    case Qt::Key_R:
        break;
    }

    updateGL();
}

void OpenGL_Widget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Invert the mouse y position because the coordinates are
        // upside down in OpenGL compared to the mouse.
        d_x = event->x();
        d_y = d_height - event->y();
    }
    if (event->button() == Qt::RightButton) {
        d_clearColor = Qt::white;
    }
    if (event->button() == Qt::MiddleButton) {
        QPoint newPos = event->pos();
        newPos.setX(newPos.x() - 50);
        newPos.setY(newPos.y() - 50);
        QCursor::setPos(QWidget::mapToGlobal(newPos));
    }
    updateGL();
}

void OpenGL_Widget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        d_clearColor = Qt::black;
    }
    if (event->button() == Qt::MiddleButton) {
        QPoint newPos = event->pos();
        newPos.setX(newPos.x() + 50);
        newPos.setY(newPos.y() + 50);
        QCursor::setPos(QWidget::mapToGlobal(newPos));
    }
    updateGL();
}

void OpenGL_Widget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        // Invert the mouse y position because the coordinates are
        // upside down in OpenGL compared to the mouse.
        d_x = event->x();
        d_y = d_height - event->y();
    } else if (event->buttons() & Qt::RightButton) {
    }
    updateGL();
}

void VRPN_API OpenGL_Widget::handleButton(void *userdata, vrpn_BUTTONCB info)
{
    OpenGL_Widget *me = static_cast<OpenGL_Widget*>(userdata);

    if (info.button == me->d_button_to_use) {
        if (info.state == 1) {
            // Just pressed our button, turn the background white.
            me->d_clearColor = Qt::white;
        }
        else {
            // Just released the button, turn the background black
            me->d_clearColor = Qt::black;
        }
    }
}

void VRPN_API OpenGL_Widget::handleAnalog(void *userdata, vrpn_ANALOGCB info)
{
    OpenGL_Widget *me = static_cast<OpenGL_Widget*>(userdata);

    vrpn_float64 value = info.channel[me->d_analog_to_use];
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);

    // If we haven't yet had an analog value set (time = 0),
    // don't check for changes, just update the value and
    // store the time we got it.
    if (me->d_analog_last_trigger.tv_sec == 0) {
        me->d_last_analog_value = value;
        me->d_analog_last_trigger = now;
        return;
    }

    // If it hasn't been long enough since our last trigger, just
    // store the value and return.
    if (vrpn_TimevalDurationSeconds(now, me->d_analog_last_trigger) < ANALOG_DELAY) {
        me->d_last_analog_value = value;
        return;
    }

    // See if the specified analog value has changed by more than the
    // threshold amount.  If so, toggle the background color and store
    // the new value we're comparing against.  Also record the time so
    // we will wait long enough before triggering again.
    if (fabs(value - me->d_last_analog_value) > me->d_analog_threshold) {
        me->d_last_analog_value = value;
        me->d_analog_last_trigger = now;
        if (me->d_clearColor == Qt::black) {
            me->d_clearColor = Qt::white;
        }
        else {
            me->d_clearColor = Qt::black;
        }
    }
}

void VRPN_API OpenGL_Widget::handleTracker(void *userdata, vrpn_TRACKERCB info)
{
    OpenGL_Widget *me = static_cast<OpenGL_Widget*>(userdata);

    struct timeval now;
    vrpn_gettimeofday(&now, NULL);

    //=======================================================================
    // If we're doing a rotation based on an axis, keep a running average of the
    // mean and standard deviation of recent measurements and adjust the brightness
    // to track within it.
    if ((me->d_tracker_rotate_axis >= 0) && (me->d_tracker_rotate_axis <= 2)) {

        // Convert the tracker's quaternion into Euler angles.  Then pull out
        // the appropraite Euler component (0, 1, or 2) so we can base our
        // calculations in it.
        q_vec_type yawPitchRoll;
        q_to_euler(yawPitchRoll, info.quat);
        double value = yawPitchRoll[me->d_tracker_rotate_axis];

        // Find the minimum and maximum rotation we've encountered.
        if (value < me->d_tracker_min_rotation) { me->d_tracker_min_rotation = value; }
        if (value > me->d_tracker_max_rotation) { me->d_tracker_max_rotation = value; }

        // Adjust the background brightness based on where we are with
        // respect to the minimum and maximum rotations
        double range = me->d_tracker_max_rotation - me->d_tracker_min_rotation;
        if (range == 0) { return; }
        double scaled = 255 * (value - me->d_tracker_min_rotation) / range;
        me->d_clearColor = QColor(scaled, scaled, scaled);

        return;
    }

    //=======================================================================
    // Not doing rotation, but rather bright/dark background based on a
    // motion threshold in translation or rotation.

    // If we haven't yet had a value set (time = 0),
    // don't check for changes, just update the location and
    // store the time we got it.
    if (me->d_tracker_last_trigger.tv_sec == 0) {
        memcpy(me->d_tracker_last_pos, info.pos, sizeof(me->d_tracker_last_pos));
        memcpy(me->d_tracker_last_quat, info.quat, sizeof(me->d_tracker_last_quat));
        me->d_tracker_last_trigger = now;
        return;
    }

    // If it hasn't been long enough since our last trigger, just
    // store the value and return.
    if (vrpn_TimevalDurationSeconds(now, me->d_tracker_last_trigger) < TRACKER_DELAY) {
        memcpy(me->d_tracker_last_pos, info.pos, sizeof(me->d_tracker_last_pos));
        memcpy(me->d_tracker_last_quat, info.quat, sizeof(me->d_tracker_last_quat));
        return;
    }

    // See if the position or rotation has changed by more than the
    // threshold amount.  If so, toggle the background color and store
    // the new value we're comparing against.  Also record the time so
    // we will wait long enough before triggering again.
    double deltaPos = q_vec_distance(info.pos, me->d_tracker_last_pos);
    double x, y, z, angle;
    q_type invert_old;
    q_invert(invert_old, me->d_tracker_last_quat);
    q_type composed;
    q_mult(composed, info.quat, invert_old);
    q_to_axis_angle(&x, &y, &z, &angle, composed);
    double deltaRot = fabs(angle);
    if ((deltaPos > me->d_tracker_trans_thresh) ||
        (deltaRot > me->d_tracker_rot_thresh)) {
        memcpy(me->d_tracker_last_pos, info.pos, sizeof(me->d_tracker_last_pos));
        memcpy(me->d_tracker_last_quat, info.quat, sizeof(me->d_tracker_last_quat));
        me->d_tracker_last_trigger = now;
        if (me->d_clearColor == Qt::black) {
            me->d_clearColor = Qt::white;
        }
        else {
            me->d_clearColor = Qt::black;
        }
    }
}

void OpenGL_Widget::setNumQuads(int num)
{
    d_num_quads = num;
    updateVertexArray();
}

// Delete any old array and make a new one based on the size.
// Fill it with two triangles per quad that together fill the
// region -1 to 1 in X and Y.  Make as many quads as we've been
// asked to.

void OpenGL_Widget::updateVertexArray(void)
{
    // It is safe to call delete[] on a NULL pointer.
    // Resize our buffer to suit.
    delete [] d_vertex_array;
    d_vertex_array = new GLfloat[d_num_quads * 3 * 6];

    // Fill in the buffer with the appropriate coordinates.
    for (int i = 0; i < d_num_quads; i++) {
        GLfloat *base = &d_vertex_array[i*3*6];

        // Make sure we get the order right, so that we get
        // front-facing triangles.

        // First coordinate, first triangle (X, Y, Z)
        *(base++) = -1; (*base++) = -1; *(base++) = 0;
        // Second coordinate, first triangle
        *(base++) =  1; (*base++) =  1; *(base++) = 0;
        // Third coordinate, first triangle
        *(base++) = -1; (*base++) =  1; *(base++) = 0;

        // First coordinate, second triangle (X, Y, Z)
        *(base++) =  1; (*base++) =  1; *(base++) = 0;
        // Second coordinate, second triangle
        *(base++) = -1; (*base++) = -1; *(base++) = 0;
        // Third coordinate, second triangle
        *(base++) =  1; (*base++) = -1; *(base++) = 0;
    }
}

