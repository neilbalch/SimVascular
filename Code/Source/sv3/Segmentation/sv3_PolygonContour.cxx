/* Copyright (c) Stanford University, The Regents of the University of
 *               California, and others.
 *
 * All Rights Reserved.
 *
 * See Copyright-SimVascular.txt for additional details.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sv3_PolygonContour.h"
#include "sv_Math.h"
#include  <cmath>

using sv3::ContourPolygon;
ContourPolygon::ContourPolygon() : Contour()
{
    m_Method="Manual";
    m_Type="Polygon";

    m_MinControlPointNumber=4;
    m_MaxControlPointNumber=200;

    m_ControlPointNonRemovableIndices[0]=0;
    m_ControlPointNonRemovableIndices[1]=1;
    //m_Extendable=true;
}

ContourPolygon::ContourPolygon(const ContourPolygon &other)
    : Contour(other)
{
}

ContourPolygon::~ContourPolygon()
{
}

ContourPolygon* ContourPolygon::Clone()
{
    return new ContourPolygon(*this);
}

std::string ContourPolygon::GetClassName()
{
    return "ContourPolygon";
}

//---------------------------
// CreateInterpolationPoints
//---------------------------
// Linear interpolate between two points.
//
std::vector<std::array<double,3> > 
CreateInterpolationPoints(std::array<double,3>  pt1, std::array<double,3>  pt2, int interNumber)
{
    std::vector<std::array<double,3> > points;

    double dx,dy,dz;
    dx=(pt2[0]-pt1[0])/interNumber;
    dy=(pt2[1]-pt1[1])/interNumber;
    dz=(pt2[2]-pt1[2])/interNumber;

    std::array<double,3>  pt;
    for(int i=1;i<interNumber;i++)
    {
        pt[0]=pt1[0]+i*dx;
        pt[1]=pt1[1]+i*dy;
        pt[2]=pt1[2]+i*dz;

        points.push_back(pt);
    }

    return points;
}

//-----------------
// SetControlPoint
//-----------------
// This method is used to 
//
//   1) Set a control point at the given location in the list of control points.
//
//   2) For index=0 (the poylgon center) then translate all control points.
//
//   3) For index=1 then scale all control points about its center.
//
void ContourPolygon::SetControlPoint(int index, std::array<double,3>  point)
{
    // Check index.
    //
    if (index >= m_ControlPoints.size()) {
        fprintf(stderr, "Unable to set control point\n");
        return;
    }

    if (index == -1) {
        index = m_ControlPoints.size() - 1;
    }

    // [TODO:DaveP] Great, let's just return without an error condition or message.
    //
    if ((index < 0) || (index > m_ControlPoints.size() - 1)) {
         return;
    }

    // Project the point onto the path plane.
    //
    double tmp[3]; 
    for (int i = 0; i < 3; i++) {
        tmp[i] = point[i];
    }

    double  projPt[3];
    m_vtkPlaneGeometry->ProjectPoint(tmp, projPt);

    // Modify the polygon center and translate all control points.
    //
    if (index == 0) {
        std::array<double,3> dirVec;
        for (int i = 0; i < 3; i++) {
            dirVec[i] = projPt[i] - m_ControlPoints[0][i];
        }
        // Translate all control points.
        Shift(dirVec);

    // Scale the polygon control points about its center.
    //
    } else if (index == 1) {
        Scale(m_ControlPoints[0], m_ControlPoints[index], std::array<double,3>{projPt[0],projPt[1],projPt[2]});

    // Replace the control point.
    //
    } else if (index < m_ControlPoints.size()) {
        m_ControlPoints[index] = std::array<double,3>{projPt[0],projPt[1],projPt[2]};
        ControlPointsChanged();
    }
}

//---------------------
// CreateContourPoints
//---------------------
// Create contour points by linearly interpolating betweent 
// control points.
//
// Modified: m_ContourPoints
//
void ContourPolygon::CreateContourPoints()
{
    int controlNumber = GetControlPointNumber();
    if (controlNumber <= 2) {
        return;
    }
    
    if (m_ContourPoints.size() != 0) {
        m_ContourPoints.clear();
    }

    if (controlNumber == 3) {
        m_ContourPoints.push_back(GetControlPoint(2));
        return;
    }

    std::vector<std::array<double,3> > tempControlPoints=m_ControlPoints;
    tempControlPoints.push_back(m_ControlPoints[2]);

    // Determine the number of interpolation points.
    //
    int interNumber;

    switch (m_SubdivisionType) {
        case CONSTANT_TOTAL_NUMBER:
            if (m_Closed) {
                interNumber=std::ceil(m_SubdivisionNumber*1.0/(controlNumber-2));
            } else {
                interNumber=std::ceil((m_SubdivisionNumber-1.0)/(controlNumber-3));
            }
        break;

        case CONSTANT_SUBDIVISION_NUMBER:
            interNumber=m_SubdivisionNumber;
        break;

        default:
        break;
    }

    // Interpolate the control points.
    //
    int controlBeginIndex = 2;

    for (int i = controlBeginIndex; i < controlNumber; i++) {
        std::array<double,3> pt1, pt2;
        pt1=tempControlPoints[i];
        pt2=tempControlPoints[i+1];
        m_ContourPoints.push_back(pt1);

        if ((i == controlNumber-1) && !m_Closed) {
            break;
        }

        if (m_SubdivisionType == CONSTANT_SPACING) {
            double dist = sqrt(pow(pt2[0]-pt1[0],2)+pow(pt2[1]-pt1[1],2)+pow(pt2[2]-pt1[2],2));
            interNumber=std::ceil(dist/m_SubdivisionSpacing);
        }

        std::vector<std::array<double,3> > interPoints=CreateInterpolationPoints(pt1,pt2,interNumber);
        m_ContourPoints.insert(m_ContourPoints.end(),interPoints.begin(),interPoints.end());
    }
}

//----------------------------------
// SearchControlPointByContourPoint
//----------------------------------
// Find the control point that equals a contour point given
// as a starting location into the list of contour points.
//
// [TODO:DaveP] This function is totally ridiculous.
//
int ContourPolygon::SearchControlPointByContourPoint(int contourPointIndex)
{
    // [TODO:DaveP] What could -2 possibly mean? 
    //
    if ((contourPointIndex < -1) || (contourPointIndex >= m_ContourPoints.size())) {
        return -2;
    }

    if (contourPointIndex == -1) {
        return m_ControlPoints.size();
    }

    int controlBeginIndex = 2;

    for (int i = contourPointIndex; i < m_ContourPoints.size(); i++) {
        auto x =  m_ContourPoints[i][0];
        auto y =  m_ContourPoints[i][1];
        auto z =  m_ContourPoints[i][2];

        for (int j = controlBeginIndex; j < m_ControlPoints.size(); j++) {
            if ((x == m_ControlPoints[j][0]) && (y == m_ControlPoints[j][1]) && (z == m_ControlPoints[j][2])) {
                return j;
            }
        }
    }

    return m_ControlPoints.size();
}

void ContourPolygon::AssignCenterScalingPoints()
{
    if(m_ControlPoints.size()>1)
    {
        m_ControlPoints[0]=m_CenterPoint;
        m_ControlPoints[1]=m_ScalingPoint;
    }
}

void ContourPolygon::PlaceControlPoints(std::array<double,3>  point)
{
    Contour::PlaceControlPoints(point);
    m_ControlPointSelectedIndex = 3;
}

ContourPolygon* ContourPolygon::CreateSmoothedContour(int fourierNumber)
{
    if(m_ContourPoints.size()<3)
        return this->Clone();

    ContourPolygon* contour=new ContourPolygon();
    contour->SetPathPoint(m_PathPoint);
    std::string method=m_Method;
    int idx=method.find("Smoothed");
    if(idx<0)
        method=method+" + Smoothed";

    contour->SetMethod(method);
    contour->SetClosed(m_Closed);

    int pointNumber=m_ContourPoints.size();

    int smoothedPointNumber;

    if((2*pointNumber)<fourierNumber)
        smoothedPointNumber=3*fourierNumber;
    else
        smoothedPointNumber=pointNumber;

    cvMath *cMath = new cvMath();
    std::vector<std::array<double, 3> > smoothedContourPoints=cMath->CreateSmoothedCurve(m_ContourPoints,m_Closed,fourierNumber,0,smoothedPointNumber);
    delete cMath;
    contour->SetContourPoints(smoothedContourPoints);

    return contour;
}
