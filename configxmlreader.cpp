/*!
 * \file configxmlreader.cpp
 *  \author Simon Coakley
 *  \date 2012
 *  \copyright Copyright (c) 2012 University of Sheffield
 *  \brief Implementation of config XML reader
 */
#include <QColor>
#include <QDebug>
#include "./configxmlreader.h"
#include "./shape.h"
#include "./position.h"

ConfigXMLReader::ConfigXMLReader(VisualSettingsModel *vsm,
        GraphSettingsModel *gsm, QString *rD, TimeScale * ts, double *r,
             float * xr, float *yr, float *xm, float * ym, float * zm,
             int * delay, float * oz, int * vd, QColor * vbg) {
    vsmodel = vsm;
    gsmodel = gsm;
    resultsData = rD;
    ratio = r;
    timeScale = ts;
    xrotate = xr;
    yrotate = yr;
    xmove = xm;
    ymove = ym;
    zmove = zm;
    delayTime = delay;
    orthoZoom = oz;
    visual_dimension = vd;
    backgroundColour = vbg;
}

bool ConfigXMLReader::read(QIODevice * device) {
    setDevice(device);

    while (!atEnd()) {
         readNext();

         if (isStartElement()) {
             if (name() == "flame_visualiser_config" &&
                     attributes().value("version") == "0.1")
                 readConfig();
             else
                 raiseError(QObject::tr(
             "The file is not an flame_visualiser_config version 0.1 file."));
         }
     }

     return !error();
}

void ConfigXMLReader::readUnknownElement() {
    Q_ASSERT(isStartElement());

     while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement())
             readUnknownElement();
     }
}

/*!
 * \brief Read flame visualiser config xml
 *
 * Read flame visualiser config xml.
 */
void ConfigXMLReader::readConfig() {
    Q_ASSERT(isStartElement() && name() == "flame_visualiser_config");

     while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "resultsData")
                 readResultsData();
             else if (name() == "timeScale")
                 readTimeScale();
             else if (name() == "animation")
                 readAnimation();
             else if (name() == "visual")
                 readVisual();
             else if (name() == "graph")
                 readGraph();
             else
                 readUnknownElement();
         }
     }
}

/*!
 * \brief Read flame visualiser config results data xml
 *
 * Read flame visualiser config results data xml.
 */
void ConfigXMLReader::readResultsData() {
    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "directory")
                 *resultsData = readElementText();
             else
                 readUnknownElement();
         }
     }
}

/*!
 * \brief Read flame visualiser config time scale xml
 *
 * Read flame visualiser config time scale xml.
 */
void ConfigXMLReader::readTimeScale() {
    QString enable;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "enable") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     timeScale->enabled = true;
                 else
                     timeScale->enabled = false;
             } else if (name() == "displayTimeInVisual") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     timeScale->displayInVisual = true;
                 else
                     timeScale->displayInVisual = false;
             } else if (name() == "milliseconds") {
                 timeScale->millisecond = readElementText().toInt();
             } else if (name() == "seconds") {
                 timeScale->second = readElementText().toInt();
             } else if (name() == "minutes") {
                 timeScale->minute = readElementText().toInt();
             } else if (name() == "hours") {
                 timeScale->hour = readElementText().toInt();
             } else if (name() == "days") {
                 timeScale->day = readElementText().toInt();
             } else {
                 readUnknownElement();
             }
         }
     }
}

/*!
 * \brief Read flame visualiser config animation xml
 *
 * Read flame visualiser config animation xml.
 */
void ConfigXMLReader::readAnimation() {
    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "delay") {
                 *delayTime = readElementText().toInt();
             } else {
                 readUnknownElement();
             }
         }
     }
}

/*!
 * \brief Read flame visualiser config visual xml
 *
 * Read flame visualiser config visual xml.
 */
void ConfigXMLReader::readVisual() {
    /* Defaults if tags missing */
    *visual_dimension = 3;
    *orthoZoom = 1.0;
    *backgroundColour = Qt::white;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "view")
                 *visual_dimension = readElementText().toInt();
             else if (name() == "ratio")
                 *ratio = readElementText().toDouble();
             else if (name() == "xrotate")
                 *xrotate = readElementText().toFloat();
             else if (name() == "yrotate")
                 *yrotate = readElementText().toFloat();
             else if (name() == "xmove")
                 *xmove = readElementText().toFloat();
             else if (name() == "ymove")
                 *ymove = readElementText().toFloat();
             else if (name() == "zmove")
                 *zmove = readElementText().toFloat();
             else if (name() == "orthoZoom")
                 *orthoZoom = readElementText().toFloat();
             else if (name() == "backgroundColour")
                 *backgroundColour = readColour();
             else if (name() == "rules")
                 readRules();
             else
                 readUnknownElement();
         }
     }
}

/*!
 * \brief Read flame visualiser config rules xml
 *
 * Read flame visualiser config rules xml.
 */
void ConfigXMLReader::readRules() {
    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "rule")
                 readRule();
             else
                 readUnknownElement();
         }
     }
}

/*!
 * \brief Read flame visualiser config rule xml
 *
 * Read flame visualiser config rule xml.
 */
void ConfigXMLReader::readRule() {
    QString agentType;
    Condition condition;
    Position x;
    Position y;
    Position z;
    Shape shape;
    QColor colour;
    bool enabled = true;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "agentType")
                 agentType = readElementText();
             else if (name() == "condition")
                 condition = readCondition();
             else if (name() == "x")
                 x  = readPosition();
             else if (name() == "y")
                 y = readPosition();
             else if (name() == "z")
                 z = readPosition();
             else if (name() == "shape")
                 shape = readShape();
             else if (name() == "colour")
                 colour = readColour();
             else if (name() == "enable") {
                 if (QString::compare(readElementText(), "true") == 0)
                     enabled = true;
                 else
                     enabled = false;
             } else {
                 readUnknownElement();
             }
         }
     }

    vsmodel->addRule(agentType, condition, x, y, z, shape, colour, enabled);
}

/*!
 * \brief Read flame visualiser config shape xml
 *
 * Read flame visualiser config shape xml.
 */
Shape ConfigXMLReader::readShape() {
    Shape shape;
    QString enable;
    bool fromCentreRead = false;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "object") {
                 shape.setShape(readElementText());
             } else if (name() == "dimension") {
                 shape.setDimension((readElementText().toDouble()));
             } else if (name() == "useVariable") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     shape.setUseVariable(true);
                 else
                     shape.setUseVariable(false);
             } else if (name() == "dimensionVariable") {
                 shape.setDimensionVariable(readElementText());
             } else if (name() == "fromCentreX") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     shape.setFromCentreX(true);
                 else
                     shape.setFromCentreX(false);
                 fromCentreRead = true;
             } else if (name() == "dimensionY") {
                 shape.setDimensionY((readElementText().toDouble()));
             } else if (name() == "useVariableY") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     shape.setUseVariableY(true);
                 else
                     shape.setUseVariableY(false);
             } else if (name() == "dimensionVariableY") {
                 shape.setDimensionVariableY(readElementText());
             } else if (name() == "fromCentreY") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     shape.setFromCentreY(true);
                 else
                     shape.setFromCentreY(false);
             } else if (name() == "dimensionZ") {
                 shape.setDimensionZ((readElementText().toDouble()));
             } else if (name() == "useVariableZ") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     shape.setUseVariableZ(true);
                 else
                     shape.setUseVariableZ(false);
             } else if (name() == "dimensionVariableZ") {
                 shape.setDimensionVariableZ(readElementText());
             } else if (name() == "fromCentreZ") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     shape.setFromCentreZ(true);
                 else
                     shape.setFromCentreZ(false);
             } else {
                 readUnknownElement();
             }
         }
     }

    /* If shape is sphere and from centre tag not read */
    if (shape.getShape() == "sphere" && fromCentreRead == false)
        shape.setFromCentreX(true);

    return shape;
}

/*!
 * \brief Read flame visualiser config position xml
 *
 * Read flame visualiser config position xml.
 */
Position ConfigXMLReader::readPosition() {
    Position position;
    QString enable;

    while (!atEnd()) {
        readNext();

        if (isEndElement())
        break;

        if (isStartElement()) {
            if (name() == "useVariable") {
                enable = readElementText();
                if (QString::compare(enable, "true") == 0)
                    position.useVariable = true;
                else
                    position.useVariable = false;
            } else if (name() == "variable") {
                position.positionVariable = readElementText();
            } else if (name() == "offSet") {
                position.opValue = readElementText().toDouble();
            } else {
                readUnknownElement();
            }
        }
    }

    return position;
}

/*!
 * \brief Read flame visualiser config condition xml
 *
 * Read flame visualiser config condition xml.
 */
Condition ConfigXMLReader::readCondition() {
    Condition condition;
    QString enable;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "enable") {
                 enable = readElementText();
                 if (QString::compare(enable, "true") == 0)
                     condition.enable = true;
                 else
                     condition.enable = false;
             } else if (name() == "lhs") {
                 condition.variable = readLhs();
             } else if (name() == "operator") {
                 condition.op = readElementText();
             } else if (name() == "rhs") {
                 condition.value = readRhs();
             } else {
                 readUnknownElement();
             }
         }
     }

    return condition;
}

/*!
 * \brief Read flame visualiser config colour xml
 *
 * Read flame visualiser config colour xml.
 */
QColor ConfigXMLReader::readColour() {
    QColor colour;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "r")
                 colour.setRed(readElementText().toInt());
             else if (name() == "g")
                 colour.setGreen(readElementText().toInt());
             else if (name() == "b")
                 colour.setBlue(readElementText().toInt());
             else if (name() == "a")
                 colour.setAlpha(readElementText().toInt());
             else
                 readUnknownElement();
         }
     }

    return colour;
}

/*!
 * \brief Read flame visualiser config lhs variable xml
 *
 * Read flame visualiser config lhs variable xml.
 */
QString ConfigXMLReader::readLhs() {
    QString agentType;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "variable")
                 agentType = readElementText();
             else
                 readUnknownElement();
         }
     }

    return agentType;
}

/*!
 * \brief Read flame visualiser config rhs value xml
 *
 * Read flame visualiser config rhs value xml.
 */
double ConfigXMLReader::readRhs() {
    double value = 0.0;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "value")
                 value = readElementText().toDouble();
             else
                 readUnknownElement();
         }
     }

    return value;
}

/*!
 * \brief Read flame visualiser config graph xml
 *
 * Read flame visualiser config graph xml.
 */
void ConfigXMLReader::readGraph() {
    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "plot")
                 readPlot();
             else
                 readUnknownElement();
         }
     }
}

/*!
 * \brief Read flame visualiser config plot xml
 *
 * Read flame visualiser config plot xml.
 */
void ConfigXMLReader::readPlot() {
    QString graphNumber;
    QString xAxis;
    QString yAxis;
    Condition condition;
    QColor colour;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "graphNumber")
                 graphNumber = readElementText();
             else if (name() == "xAxis")
                 xAxis = readXaxis();
             else if (name() == "yAxis")
                 yAxis  = readYaxis();
             else if (name() == "condition")
                 condition = readCondition();
             else if (name() == "colour")
                 colour = readColour();
             else
                 readUnknownElement();
         }
     }

    gsmodel->addPlot(graphNumber, xAxis, yAxis, condition, colour, false);
}

/*!
 * \brief Read flame visualiser config x-axis xml
 *
 * Read flame visualiser config x-axis xml.
 */
QString ConfigXMLReader::readXaxis() {
    QString type;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "type")
                 type = readElementText();
             else
                 readUnknownElement();
         }
     }

    return type;
}

/*!
 * \brief Read flame visualiser config y-axis xml
 *
 * Read flame visualiser config y-axis xml.
 */
QString ConfigXMLReader::readYaxis() {
    QString agentType;

    while (!atEnd()) {
         readNext();

         if (isEndElement())
             break;

         if (isStartElement()) {
             if (name() == "agentType")
                 agentType = readElementText();
             else
                 readUnknownElement();
         }
     }

    return agentType;
}
