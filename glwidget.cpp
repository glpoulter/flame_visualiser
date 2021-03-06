/*!
 * \file glwidget.cpp
 *  \author Simon Coakley
 *  \date 2012
 *  \copyright Copyright (c) 2012 University of Sheffield
 *  \brief Implementation of OpenGL widget
 */
#include <QtGui>
#include <QKeyEvent>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QMessageBox>
#include <QtPlugin>
#include "./glwidget.h"
#include "./visualsettingsmodel.h"
#include "./visualsettingsitem.h"
#include "./condition.h"
#include "./agentdialog.h"

GLWidget::GLWidget(float * xr, float * yr, float * xm, float * ym, float * zm,
        Dimension * rd, float * oz, bool *ani, QWidget *parent)
    : QGLWidget(parent) {
    agents = 0;
    setMouseTracking(true);
    xrotate = xr;
    yrotate = yr;
    xmove = xm;
    ymove = ym;
    zmove = zm;
    dimension = 3;
    x_last_position = 0;
    y_last_position = 0;
    drawpoints = 1;
    light = true;
    alphaA = 1.0;
    spinup = false;
    spindown = false;
    spinleft = false;
    spinright = false;
    itforward = false;
    itback = false;
    block = false;
    animation = ani;
    locked = false;
    animationImages = false;
    imageLock = false;
    pickOn = false;
    zNear = 0.1f;
    clippingOn = false;
    drawNameAgent = false;
    moveOn = false;
    restrictDimension = rd;
    restrictAxesOn = false;
    delayLock = false;
    delayTime = 0;
    orthoZoom = oz;
    background = Qt::white;

    time = QTime::currentTime();
    timer = new QTimer(this);
    timer->setSingleShot(true); /* timer only fires once */
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));

    delayTimer = new QTimer(this);
    delayTimer->setSingleShot(true); /* timer only fires once */
    connect(delayTimer, SIGNAL(timeout()), this, SLOT(unlockDelayTime()));

    // To instruct OpenGL not to paint a background for the widget
    // when QPainter::begin() is called.
    setAutoFillBackground(false);
}

GLWidget::~GLWidget() {
    // Delete display lists
    glDeleteLists(nPartsList, 5);
    emit(visual_window_closed());
}

void GLWidget::closeEvent(QCloseEvent *event) {
    emit(visual_window_closed());
    event->accept();
}

void GLWidget::setBackgroundColour(QColor b) {
    background = b;
    initializeGL();
}

void GLWidget::updateDelayTime(int d) {
    delayTime = d;
    // qDebug() << "glwidget delayTime = " << delayTime;
}

void GLWidget::unlockDelayTime() {
    delayLock = false;
}

void GLWidget::updateImagesLocation(QString s) {
    imagesLocation = s;
}

void GLWidget::setDimension(int d) {
    dimension = d;
    if (d == 2) resize(600, 600);
    if (d == 3) resize(800, 600);
}

void GLWidget::takeSnapshot() {
    imageLock = true;

    QString filepath;
    filepath.append(*configpath);
    filepath.append("/");
    filepath.append(imagesLocation);

    QDir filedir(filepath);

    QString filename = filedir.canonicalPath();
    filename.append("/");
    filename.append(QString("%1").arg(*iteration));
    filename.append(".jpg");

    QImage i = this->grabFrameBuffer();
    QImageWriter writer(filename, "JPG");
    writer.setQuality(90);
    if (!writer.write(i)) {
        qDebug() << writer.errorString() << filename;
        emit(imageStatus(QString("Error: %1 %2").
                arg(writer.errorString(), filename)));
    } else {
        emit(imageStatus(QString("Saved: %1").arg(filename)));
    }
    imageLock = false;
}

void GLWidget::takeAnimation(bool b) {
    animationImages = b;
}

void GLWidget::iterationLoaded() {
    locked = false;
}

void GLWidget::nextIteration() {
    emit(increase_iteration());
}

void GLWidget::update_agents(QList<Agent *> *a) {
    agents = a;
}

void GLWidget::set_rules(VisualSettingsModel * m) {
    model = m;
}

void GLWidget::reset_camera() {
    *xrotate = 0.0;
    *yrotate = 0.0;
    *xmove = 0.0;
    *ymove = 0.0;
    *zmove = -3.0;
    *orthoZoom = 1.0;
    if (dimension == 2)
        resizeGL(windowWidth, windowHeight);
}

void GLWidget::initializeGL() {
    /* Reflectivness properties of material */
    GLfloat mat_specular[] = { 0.0, 0.0, 0.0, 1.0 };
    /* Size & Brightness of highlight, 0-128 */
    GLfloat mat_shininess[] = { 50.0 };
    /* Ambient light from the light source */
    GLfloat light_ambient[] =  { 0.0, 0.0, 0.0, 1.0 };
    /* Colour of the light from light source */
    GLfloat light_diffuse[] =  { 1.0, 1.0, 1.0, 1.0 };
    /* Colour of highlight on object, should use same as diffuse */
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
    /* Light source position */
    GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
    /* Global ambient light, use this to lighten objects */
    GLfloat ambient_light[] =  { 0.6, 0.6, 0.6, 1.0 };
    /* Global ambient light, use this to lighten objects */
    // GLfloat ambient_light[] =  { 1.0, 1.0, 1.0, 1.0 };

    // glDisable(GL_TEXTURE_2D);
    // glDisable(GL_DEPTH_TEST);
    // glDisable(GL_COLOR_MATERIAL);
    // glEnable(GL_BLEND);
    // glEnable(GL_POLYGON_SMOOTH);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(background.redF(), background.greenF(), background.blueF(), 0);
    glShadeModel(GL_SMOOTH);
    glClearDepth(1.0f);
    // glEnable( GL_DEPTH_TEST );
    // glDepthFunc( GL_LEQUAL );
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    /* Lighting */
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient_light);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Display lists
    nPartsList = glGenLists(5);
    SPHERE_4  = nPartsList;
    SPHERE_8  = nPartsList+1;
    SPHERE_16 = nPartsList+2;
    SPHERE_32 = nPartsList+3;
    SPHERE_64 = nPartsList+4;
    dl_qobj = gluNewQuadric();
    glNewList(SPHERE_4, GL_COMPILE);
        gluSphere(dl_qobj, 1.0, 4, 4);
    glEndList();
    glNewList(SPHERE_8, GL_COMPILE);
        gluSphere(dl_qobj, 1.0, 8, 8);
    glEndList();
    glNewList(SPHERE_16, GL_COMPILE);
        gluSphere(dl_qobj, 1.0, 16, 16);
    glEndList();
    glNewList(SPHERE_32, GL_COMPILE);
        gluSphere(dl_qobj, 1.0, 32, 32);
    glEndList();
    glNewList(SPHERE_64, GL_COMPILE);
        gluSphere(dl_qobj, 1.0, 64, 64);
    glEndList();
}

void GLWidget::resizeGL(int w, int h) {
    window_ratio = (GLfloat)(static_cast<float>(w)/static_cast<float>(h));
    windowWidth = w;
    windowHeight = h;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (dimension == 2) {
        glViewport(0, 0, qMax(w, h), qMax(w, h));
        glOrtho(-1*(*orthoZoom), 1*(*orthoZoom),
                -1*(*orthoZoom), 1*(*orthoZoom), zNear, 20);
    }
    if (dimension == 3) {
        gluPerspective(45.0f, window_ratio, zNear, 20.0f);
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void GLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    glTranslatef(*xmove, *ymove, *zmove);
    glRotatef(*yrotate, 1.0f, 0.0f, 0.0f);
    glRotatef(*xrotate, 0.0f, 0.0f, 1.0f);

    // Does not include translation and rotation of scene
    // at the moment so leave out
    // ExtractFrustum();

    drawAgents(GL_RENDER);

    glPopMatrix();
}

void GLWidget::drawSphere(double size) {
    if (size > 0.4)
        glCallList(SPHERE_64);
    else
        glCallList(SPHERE_16);
}

void GLWidget::drawAgents(GLenum mode) {
    double size = 0.0;
    double sizeY = 0.0;
    double sizeZ = 0.0;
    // int grade;
    int name = 0;
    GLfloat mat_ambientA[4];
    nameAgents.clear();
    /*int style = 0;
    GLUquadricObj * qobj = gluNewQuadric();

    if (style == 0) {
        gluQuadricDrawStyle(qobj, GLU_FILL); // smooth shaded
        gluQuadricNormals(qobj, GLU_SMOOTH);
    } else if (style == 1) {
        gluQuadricDrawStyle(qobj, GLU_FILL); // flat shaded
        gluQuadricNormals(qobj, GLU_FLAT);
    } else if (style == 2) {
        gluQuadricDrawStyle(qobj, GLU_LINE); // all polygons wireframe
        gluQuadricNormals(qobj, GLU_NONE);
    }*/

    for (int pass = 1; pass < 4; pass++) {
        if (pass == 1) {
            // if first pass
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDisable(GL_BLEND);
        } else if (pass == 2) {
            // if second pass
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        /* For every rule */
        for (int j = 0; j < model->getRules().count(); j++) {
            VisualSettingsItem * rule = model->getRules()[j];

            /* For every agent associated with the current rule */
            for (int i = 0; i < rule->agents.size(); i++) {
                /* Check the restrict dimensions */
                if (!restrictAxesOn || (restrictAxesOn &&
                ( (!restrictDimension->xminon ||
                        (restrictDimension->xminon && rule->agents.at(i)->x >
                            restrictDimension->xmin)) &&
                    (!restrictDimension->xmaxon ||
                        (restrictDimension->xmaxon && rule->agents.at(i)->x <
                            restrictDimension->xmax)) &&
                    (!restrictDimension->yminon ||
                        (restrictDimension->yminon && rule->agents.at(i)->y >
                            restrictDimension->ymin)) &&
                    (!restrictDimension->ymaxon ||
                        (restrictDimension->ymaxon && rule->agents.at(i)->y <
                            restrictDimension->ymax)) &&
                    (!restrictDimension->zminon ||
                        (restrictDimension->zminon && rule->agents.at(i)->z >
                            restrictDimension->zmin)) &&
                    (!restrictDimension->zmaxon ||
                        (restrictDimension->zmaxon && rule->agents.at(i)->z <
                            restrictDimension->zmax)) ))) {
                    /* If the object is opaque and first pass, or
                       if the object is transparent and is the second or third pass */
                    if (((rule->colour().alphaF() >= 0.95 && pass == 1) ||
                            (rule->colour().alphaF() < 0.95 && pass >= 2))) {
                        glPushMatrix();

                        glTranslatef(rule->agents.at(i)->x,
                                     rule->agents.at(i)->y,
                                     rule->agents.at(i)->z);

                        if (rule->agents.at(i)->isPicked) {
                            mat_ambientA[0] = 1.0-rule->colour().redF();
                            mat_ambientA[1] = 1.0-rule->colour().greenF();
                            mat_ambientA[2] = 1.0-rule->colour().blueF();
                            mat_ambientA[3] = rule->colour().alphaF();
                        } else {
                            mat_ambientA[0] = rule->colour().redF();
                            mat_ambientA[1] = rule->colour().greenF();
                            mat_ambientA[2] = rule->colour().blueF();
                            mat_ambientA[3] = rule->colour().alphaF();
                        }

                        if (light)
                            glMaterialfv(GL_FRONT,
                                    GL_AMBIENT_AND_DIFFUSE, mat_ambientA);
                        else
                            glColor4fv(mat_ambientA);

                        size = rule->agents.at(i)->shapeDimension/2.0;
                        sizeY = rule->agents.at(i)->shapeDimensionY/2.0;
                        sizeZ = rule->agents.at(i)->shapeDimensionZ/2.0;

                        if (QString::compare("sphere", rule->shape().
                                getShape()) == 0) {
                            glEnable(GL_CULL_FACE);
                            if (pass == 2) {
                                if (mode == GL_SELECT) glLoadName(0);
                                // Draw back face
                                glCullFace(GL_FRONT);
                                glPushMatrix();
                                glScalef(size, size, size);
                                glEnable(GL_NORMALIZE);
                                drawSphere(size);
                                glDisable(GL_NORMALIZE);
                                glPopMatrix();
                            } else {
                                if (mode == GL_SELECT) {
                                    glLoadName(++name);
                                    nameAgents.insert(name, rule->agents.at(i));
                                }
                                // Draw front face
                                glCullFace(GL_BACK);
                                glPushMatrix();
                                glScalef(size, size, size);
                                glEnable(GL_NORMALIZE);
                                drawSphere(size);
                                glDisable(GL_NORMALIZE);
                                glPopMatrix();
                            }
                            glDisable(GL_CULL_FACE);
                        } else if (QString::compare("point", rule->shape().
                                getShape()) == 0) {
                            if (pass != 2) {
                                if (mode == GL_SELECT) {
                                    glLoadName(++name);
                                    nameAgents.insert(name, rule->agents[i]);
                                }

                                glDisable(GL_LIGHTING);
                                glColor4fv(mat_ambientA);
                                glPointSize(static_cast<int>
                                    (rule->agents.at(i)->shapeDimension));
                                glBegin(GL_POINTS);
                                glVertex3f(0.0, 0.0, 0.0);
                                glEnd();
                                glEnable(GL_LIGHTING);
                            }
                        } else if (QString::compare("cube", rule->shape().
                                getShape()) == 0) {
                            glEnable(GL_CULL_FACE);
                            if (pass == 2) {
                                if (mode == GL_SELECT) glLoadName(0);
                                // Draw back face
                                glCullFace(GL_FRONT);
                                drawCube(size, sizeY, sizeZ);
                            } else {
                                if (mode == GL_SELECT) {
                                    glLoadName(++name);
                                    nameAgents.insert(name, rule->agents[i]);
                                }
                                // Draw front face
                                glCullFace(GL_BACK);
                                drawCube(size, sizeY, sizeZ);
                            }
                            glDisable(GL_CULL_FACE);
                        }

                        // if(mode == GL_SELECT) glPopName();
                        glPopMatrix();
                    }
                }
            }
        }
    }
}

void GLWidget::drawCube(float size, float sizeY, float sizeZ) {
    glBegin(GL_QUADS);  // Draw using quads, T-Top B-Bottom
    glNormal3f(0.0f, 0.5f, 0.0f);
    glVertex3f(size, sizeY, -sizeZ);  // T-right of the quad (T)
    glVertex3f(-size, sizeY, -sizeZ);  // T-left of the quad (T)
    glVertex3f(-size, sizeY,  sizeZ);  // B-left of the quad (T)
    glVertex3f(size, sizeY,  sizeZ);  // B-right of the quad (T)

    glNormal3f(0.0f, -0.5f, 0.0f);
    glVertex3f(size, -sizeY,  sizeZ);  // T-right of the quad (B)
    glVertex3f(-size, -sizeY,  sizeZ);  // T-left of the quad (B)
    glVertex3f(-size, -sizeY, -sizeZ);  // B-left of the quad (B)
    glVertex3f(size, -sizeY, -sizeZ);  // B-right of the quad (B)

    glNormal3f(0.0f, 0.0f, 0.5f);
    glVertex3f(size,  sizeY, sizeZ);  // T-right of the quad (F)
    glVertex3f(-size,  sizeY, sizeZ);  // T-left of the quad (F)
    glVertex3f(-size, -sizeY, sizeZ);  // B-left of the quad (F)
    glVertex3f(size, -sizeY, sizeZ);  // B-right of the quad (F)

    glNormal3f(0.0f, 0.0f, -0.5f);
    glVertex3f(size, -sizeY, -sizeZ);  // B-left of the quad (B)
    glVertex3f(-size, -sizeY, -sizeZ);  // B-right of the quad (B)
    glVertex3f(-size,  sizeY, -sizeZ);  // T-right of the quad (B)
    glVertex3f(size,  sizeY, -sizeZ);  // T-left of the quad (B)

    glNormal3f(-0.5f, 0.0f, 0.0f);
    glVertex3f(-size,  sizeY,  sizeZ);  // T-right of the quad (L)
    glVertex3f(-size,  sizeY, -sizeZ);  // T-left of the quad (L)
    glVertex3f(-size, -sizeY, -sizeZ);  // B-left of the quad (L)
    glVertex3f(-size, -sizeY,  sizeZ);  // B-right of the quad (L)

    glNormal3f(0.5f, 0.0f, 0.0f);
    glVertex3f(size,  sizeY, -sizeZ);  // T-right of the quad (R)
    glVertex3f(size,  sizeY,  sizeZ);  // T-left of the quad (R)
    glVertex3f(size, -sizeY,  sizeZ);  // B-left of the quad (R)
    glVertex3f(size, -sizeY, -sizeZ);  // B-right of the quad (R)
    glEnd();   // Done drawing the color cube
}

void GLWidget::paintEvent(QPaintEvent */*event*/) {
    paintGL();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(QFont("Courier", 20));

    if (timeScale->enabled && timeScale->displayInVisual) {
        const QRect bbox(painter.boundingRect(QRect(0, 0, 0, 0),
                Qt::AlignLeft, *timeString));
        QSize windowSize = size();

        painter.drawText(windowSize.width()-bbox.width()-15,
                windowSize.height()-bbox.height(), *timeString);
    }

    if (drawNameAgent) {
        for (int i = 0; i < nameAgent.tags.size(); i++) {
            painter.drawText(10, 20*(i+1), QString("%1\t%2").
                    arg(nameAgent.tags[i], nameAgent.values[i]));
        }
    }

    painter.end();

    glFlush();

    if (spinup)    *yrotate -= 1.0;
    if (spindown)  *yrotate += 1.0;
    if (spinleft)  *xrotate -= 1.0;
    if (spinright) *xrotate += 1.0;

    // Framerate control
    int delay = time.msecsTo(QTime::currentTime());
    if (delay == 0) delay = 1;
    time = QTime::currentTime();
    /* Start the timer with timeout units of milliseconds */
    timer->start(qMax(0, 20 - delay));

    if (*animation && !locked && !imageLock && !delayLock) {
        if (delayTime > 0) {
            delayLock = true;
            delayTimer->start(delayTime);
        }
        if (animationImages) takeSnapshot();
        emit(increase_iteration());
    }
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
    x_last_position = event->x();
    y_last_position = event->y();
    if (pickOn) processSelection(x_last_position, y_last_position);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
    // if left button
    if (event->buttons() & Qt::LeftButton) {
        if (moveOn) {
            *xmove -= static_cast<float>((x_last_position - event->x())/90.0);
            *ymove += static_cast<float>((y_last_position - event->y())/90.0);
        } else {
            *xrotate -= static_cast<float>((x_last_position - event->x())/2.0);
            *yrotate -= static_cast<float>((y_last_position - event->y())/2.0);
        }
    } else if (event->buttons() & Qt::RightButton) {
        if (clippingOn) {
            zNear += static_cast<float>((y_last_position - event->y())/100.0);
            if (zNear < 0.2) zNear = 0.2;
            resizeGL(windowWidth, windowHeight);
        } else {
            if (dimension == 2) {
                *orthoZoom -=
                    static_cast<float>((y_last_position - event->y())/100.0);
                resizeGL(windowWidth, windowHeight);
            }
            if (dimension == 3)
            *zmove += static_cast<float>((y_last_position - event->y())/100.0);
        }
    }

    x_last_position = event->x();
    y_last_position = event->y();
    update();
}

void GLWidget::wheelEvent(QWheelEvent * event) {
    if (dimension == 2) {
        *orthoZoom -= static_cast<float>(event->delta()/500.0);
        resizeGL(windowWidth, windowHeight);
    }
    if (dimension == 3) {
        *zmove += static_cast<float>(event->delta()/500.0);
    }
    update();
}

void GLWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Escape:
            close();
            break;
        case Qt::Key_Left:
            spinleft = true;
            break;
        case Qt::Key_Right:
            spinright = true;
            break;
        case Qt::Key_Up:
            spinup = true;
            break;
        case Qt::Key_Down:
            spindown = true;
            break;
        case Qt::Key_Z:
            emit(decrease_iteration());
            break;
        case Qt::Key_X:
            emit(increase_iteration());
            break;
        case Qt::Key_A:
            emit(signal_toggleAnimation());
            break;
        case Qt::Key_P:
            pickOn = true;
            break;
        case Qt::Key_C:
            clippingOn = true;
            break;
        case Qt::Key_Space:
            moveOn = true;
            break;
        default:
            event->ignore();
            break;
    }
}

void GLWidget::keyReleaseEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Left:
            spinleft = false;
            break;
        case Qt::Key_Right:
            spinright = false;
            break;
        case Qt::Key_Up:
            spinup = false;
            break;
        case Qt::Key_Down:
            spindown = false;
            break;
        case Qt::Key_P:
            pickOn = false;
            break;
        case Qt::Key_C:
            clippingOn = false;
            break;
        case Qt::Key_Space:
            moveOn = false;
            break;
        default:
            event->ignore();
            break;
    }
}

/* The buffer size for picking objects */
#define BUFSIZE 512
void GLWidget::processSelection(int mx, int my) {
    GLuint select_buf[BUFSIZE];  // selection buffer
    GLint hits;  // number of hits
    GLint  viewport[4];

    glGetIntegerv(GL_VIEWPORT, viewport);

    glSelectBuffer(BUFSIZE, select_buf);
    glRenderMode(GL_SELECT);
    glInitNames();
    glPushName(0);

    // glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPickMatrix((GLdouble) mx, (GLdouble) (viewport[3] - my),
            5.0, 5.0, viewport);
    gluPerspective(45.0f, window_ratio, zNear, 20.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix();

    glTranslatef(*xmove, *ymove, *zmove);
    glRotatef(*yrotate, 1.0f, 0.0f, 0.0f);
    glRotatef(*xrotate, 0.0f, 0.0f, 1.0f);

    drawAgents(GL_SELECT);

    glPopMatrix();

    hits = glRenderMode(GL_RENDER);

    drawNameAgent = false;

    int i, j;
    int nitems, zmin, item = 0;
    //  int zmax;
    int minimumDepth = 0;
    int pickItem = -1;

    // qDebug() << "hits = " << hits;
    int index = 0;
    for (i = 0; i < hits; i++) {
        nitems = select_buf[index++];
        zmin = select_buf[index++];
        //  zmax = select_buf[index++];
        index++;

        // qDebug() << zmin << zmax;
        for (j = 0; j < nitems; j++) {
            item = select_buf[index++];
            // qDebug() << j << item ;
        }

        /* Ignore item=0 as these are the backside of objects if transparent */
        if (item > 0) {
            // qDebug() << item << zmin << zmax;

            if (i == 0) {
                minimumDepth = zmin;
                pickItem = item;
            } else if (zmin < minimumDepth) {
                minimumDepth = zmin;
                pickItem = item;
            }
        }
    }
    if (pickItem != -1) {
        RuleAgent * a = nameAgents.value(pickItem);
        nameAgent.tags = QStringList(a->agent->tags);
        nameAgent.values = QStringList(a->agent->values);
        drawNameAgent = false;  // true;

        a->isPicked = true;

        /* Release of button p is not caught because the focus is given to the
         * agent dialog */
        pickOn = false;
        /* Create dialog */
        AgentDialog * agentDialog = new AgentDialog(a->agent, this);
        agentDialog->show();
    }

    resizeGL(windowWidth, windowHeight);
}

void GLWidget::restrictAxes(bool b) {
    restrictAxesOn = b;
}

float GLWidget::SphereInFrustum(float x, float y, float z, float radius) {
    int p;
    float d;

    for (p = 0; p < 6; p++) {
        d = frustum[p][0] * x + frustum[p][1] * y +
            frustum[p][2] * z + frustum[p][3];
        if (d <= -radius) return 0;
    }
    return d + radius;
}

void GLWidget::ExtractFrustum() {
    float   proj[16];
    float   modl[16];
    float   clip[16];
    float   t;

    /* Get the current PROJECTION matrix from OpenGL */
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    /* Get the current MODELVIEW matrix from OpenGL */
    glGetFloatv(GL_MODELVIEW_MATRIX, modl);

    /* Combine the two matrices (multiply projection by modelview) */
    clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] +
               modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
    clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] +
               modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
    clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] +
               modl[ 2] * proj[10] + modl[ 3] * proj[14];
    clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] +
               modl[ 2] * proj[11] + modl[ 3] * proj[15];

    clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] +
               modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
    clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] +
               modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
    clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] +
               modl[ 6] * proj[10] + modl[ 7] * proj[14];
    clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] +
               modl[ 6] * proj[11] + modl[ 7] * proj[15];

    clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] +
               modl[10] * proj[ 8] + modl[11] * proj[12];
    clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] +
               modl[10] * proj[ 9] + modl[11] * proj[13];
    clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] +
               modl[10] * proj[10] + modl[11] * proj[14];
    clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] +
               modl[10] * proj[11] + modl[11] * proj[15];

    clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] +
               modl[14] * proj[ 8] + modl[15] * proj[12];
    clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] +
               modl[14] * proj[ 9] + modl[15] * proj[13];
    clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] +
               modl[14] * proj[10] + modl[15] * proj[14];
    clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] +
               modl[14] * proj[11] + modl[15] * proj[15];

    /* Extract the numbers for the RIGHT plane */
    frustum[0][0] = clip[ 3] - clip[ 0];
    frustum[0][1] = clip[ 7] - clip[ 4];
    frustum[0][2] = clip[11] - clip[ 8];
    frustum[0][3] = clip[15] - clip[12];

    /* Normalize the result */
    t = sqrt(frustum[0][0] * frustum[0][0] + frustum[0][1] *
             frustum[0][1] + frustum[0][2] * frustum[0][2]);
    frustum[0][0] /= t;
    frustum[0][1] /= t;
    frustum[0][2] /= t;
    frustum[0][3] /= t;

    /* Extract the numbers for the LEFT plane */
    frustum[1][0] = clip[ 3] + clip[ 0];
    frustum[1][1] = clip[ 7] + clip[ 4];
    frustum[1][2] = clip[11] + clip[ 8];
    frustum[1][3] = clip[15] + clip[12];

    /* Normalize the result */
    t = sqrt(frustum[1][0] * frustum[1][0] + frustum[1][1] *
             frustum[1][1] + frustum[1][2] * frustum[1][2]);
    frustum[1][0] /= t;
    frustum[1][1] /= t;
    frustum[1][2] /= t;
    frustum[1][3] /= t;

    /* Extract the BOTTOM plane */
    frustum[2][0] = clip[ 3] + clip[ 1];
    frustum[2][1] = clip[ 7] + clip[ 5];
    frustum[2][2] = clip[11] + clip[ 9];
    frustum[2][3] = clip[15] + clip[13];

    /* Normalize the result */
    t = sqrt(frustum[2][0] * frustum[2][0] + frustum[2][1] *
             frustum[2][1] + frustum[2][2] * frustum[2][2]);
    frustum[2][0] /= t;
    frustum[2][1] /= t;
    frustum[2][2] /= t;
    frustum[2][3] /= t;

    /* Extract the TOP plane */
    frustum[3][0] = clip[ 3] - clip[ 1];
    frustum[3][1] = clip[ 7] - clip[ 5];
    frustum[3][2] = clip[11] - clip[ 9];
    frustum[3][3] = clip[15] - clip[13];

    /* Normalize the result */
    t = sqrt(frustum[3][0] * frustum[3][0] + frustum[3][1] *
             frustum[3][1] + frustum[3][2] * frustum[3][2]);
    frustum[3][0] /= t;
    frustum[3][1] /= t;
    frustum[3][2] /= t;
    frustum[3][3] /= t;

    /* Extract the FAR plane */
    frustum[4][0] = clip[ 3] - clip[ 2];
    frustum[4][1] = clip[ 7] - clip[ 6];
    frustum[4][2] = clip[11] - clip[10];
    frustum[4][3] = clip[15] - clip[14];

    /* Normalize the result */
    t = sqrt(frustum[4][0] * frustum[4][0] + frustum[4][1] *
             frustum[4][1] + frustum[4][2] * frustum[4][2]);
    frustum[4][0] /= t;
    frustum[4][1] /= t;
    frustum[4][2] /= t;
    frustum[4][3] /= t;

    /* Extract the NEAR plane */
    frustum[5][0] = clip[ 3] + clip[ 2];
    frustum[5][1] = clip[ 7] + clip[ 6];
    frustum[5][2] = clip[11] + clip[10];
    frustum[5][3] = clip[15] + clip[14];

    /* Normalize the result */
    t = sqrt(frustum[5][0] * frustum[5][0] + frustum[5][1] *
             frustum[5][1] + frustum[5][2] * frustum[5][2]);
    frustum[5][0] /= t;
    frustum[5][1] /= t;
    frustum[5][2] /= t;
    frustum[5][3] /= t;
}
