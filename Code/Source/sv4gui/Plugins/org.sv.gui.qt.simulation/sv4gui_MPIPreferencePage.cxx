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

// The sv4guiMPIPreferencePage class is used to process information 
// about the location of the solver binaries (svpre, svsolver and svpost) 
// and the mpiexec binary used to execute a simulation presented in the 
// 'Preferences->SimVascular Simulation' panel. 
//
// sv4guiMPIPreferencePage methods are used to 
//
//     1) Process GUI events 
//
//     2) Set the values of the solver binaries in the MITK database 
//
// The MITK database provides persistence for solver binary values between 
// SimVascular sessions.
//
// The values of the solver binaries are set from the MITK database if it 
// exists. Otherwise they are set using their default values obtained using
// an sv4guiMPIPreferences() object.
//
// Pressing the SimVascular Simulation' panel 'OK' button calls the PerformOk() 
// method which saves the solver binary values into the MITK database.
//  

#include "sv4gui_MPIPreferencePage.h"
#include "ui_sv4gui_MPIPreferencePage.h"

#include <berryIPreferencesService.h>
#include <berryPlatform.h>

#include <mitkExceptionMacro.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>

//--------------------------------
// sv4guiMPIPreferencePage
//--------------------------------
// Constructor.
//
sv4guiMPIPreferencePage::sv4guiMPIPreferencePage() : m_Preferences(nullptr), 
    m_Ui(new Ui::sv4guiMPIPreferencePage) , m_Control(nullptr)
{
  // Set the default locations of the solver binaries.
  m_DefaultPrefs = sv4guiMPIPreferences();
}

sv4guiMPIPreferencePage::~sv4guiMPIPreferencePage()
{
}

//---------------------------
// InitializeSolverLocations
//---------------------------
// Find the location of solver binaries and mpiexec.
//
// The the full binary path is displayed in the SimVascular 
// 'Preferences->SimVascular Simulations' page and used to 
// execute a simulation.
//
// If the values for the binaries and mpiexec are not already
// set in the SimVascular MITK database then they set to their
// default values set in the sv4guiMPIPreferences method.
//
void sv4guiMPIPreferencePage::InitializeMPILocation()
{
  // Set the mpiexec binary.
  SetMpiExec(); 

  // Set the MPI implementation. 
  SetMpiImplementation();
}

//------------
// SetMpiExec
//------------
// Set the location of the MPI mpiexec binary.
//
void sv4guiMPIPreferencePage::SetMpiExec()
{
  QString mpiExec = m_Ui->lineEditMPIExecPath->text().trimmed();

  if (!mpiExec.isEmpty() && (mpiExec != m_DefaultPrefs.UnknownBinary)) {
    return;
  }

  mpiExec = m_DefaultPrefs.GetMpiExec();
  m_Ui->lineEditMPIExecPath->setText(mpiExec);
}

//----------------------
// SetMpiImplementation 
//----------------------
// Set the installed MPI implementation.
//
void sv4guiMPIPreferencePage::SetMpiImplementation()
{
  auto msg = "[sv4guiMPIPreferencePage::SetMpiImplementation] ";
  std::cout << msg << std::endl; 
  std::cout << msg << "####################################################" << std::endl; 
  std::cout << msg << "               SetMpiImplementation                 " << std::endl; 
  std::cout << msg << "####################################################" << std::endl; 

  QString mpiExec = m_Ui->lineEditMPIExecPath->text().trimmed();
  std::cout << msg << "mpiExec: " << mpiExec.toStdString() << std::endl; 

  if (mpiExec.isEmpty() || (mpiExec == m_DefaultPrefs.UnknownBinary)) {
    return;
  }

  QString guiLabel("MPI Implementation: ");
  auto implStr = m_DefaultPrefs.GetMpiName();
  std::cout << msg << "implStr: " << implStr.toStdString() << std::endl; 
  m_Ui->labelMPIImplementation->setText(guiLabel + implStr);
}

//-----------------
// CreateQtControl
//-----------------
//
void sv4guiMPIPreferencePage::CreateQtControl(QWidget* parent)
{
    m_Control = new QWidget(parent);

    m_Ui->setupUi(m_Control);

    berry::IPreferencesService* prefService = berry::Platform::GetPreferencesService();
    Q_ASSERT(prefService);

    m_Preferences = prefService->GetSystemPreferences()->Node("/org.sv.views.simulation");
    connect( m_Ui->toolButtonMPIExec, SIGNAL(clicked()), this, SLOT(SelectMPIExecPath()) );
    connect(m_Ui->lineEditMPIExecPath, SIGNAL(returnPressed()), this, SLOT(SetMPIExecPath()) ); 
    //connect(m_Ui->lineEditMPIExecPath, SIGNAL(selectionChanged()), this, SLOT(SetMPIExecPath()) ); 
    //connect(m_Ui->lineEditMPIExecPath, SIGNAL(textChanged(QString)), this, SLOT(SetMPIExecPath()) ); 

    this->Update();

    // Set the locations of the solver binaries and mpiexec.
    InitializeMPILocation();
}

//-------------------
// SelectMPIExecPath
//-------------------
//
void sv4guiMPIPreferencePage::SelectMPIExecPath()
{
    QString filePath = QFileDialog::getOpenFileName(m_Control, "Choose MPIExec");

    if (!filePath.isEmpty()) {
        m_Ui->lineEditMPIExecPath->setText(filePath);
        SetMPIExecPath();
    }
}

//----------------
// SetMPIExecPath
//----------------
// Process the GUI event to set the mpiexec path.
//
void sv4guiMPIPreferencePage::SetMPIExecPath()
{
    auto msg = "[sv4guiMPIPreferencePage::SetMPIExecPath] ";
    MITK_INFO << msg;
    MITK_INFO << msg << "------------------------------ SetMPIExecPath -----------------------";
    auto filePath = m_Ui->lineEditMPIExecPath->text().trimmed();
    MITK_INFO << msg << "filePath: " << filePath.toStdString();

    if (!filePath.isEmpty()) {
        m_DefaultPrefs.SetMpiImplementation(filePath);
        SetMpiImplementation();
    }
}

QWidget* sv4guiMPIPreferencePage::GetQtControl() const
{
    return m_Control;
}

void sv4guiMPIPreferencePage::Init(berry::IWorkbench::Pointer)
{
}

void sv4guiMPIPreferencePage::PerformCancel()
{
}

//-----------
// PerformOk
//-----------
// Process the 'OK' button GUI event.
//
bool sv4guiMPIPreferencePage::PerformOk()
{
    // Get the solver paths from the GUI
    bool useMPI = m_Ui->checkBoxUseMPI->isChecked();
    QString MPIExecPath = m_Ui->lineEditMPIExecPath->text().trimmed();

    // Set the values of the solver paths in the MITK database.
    m_Preferences->PutBool("use mpi", useMPI);
    
    // MPI implementation.
    m_Preferences->Put("mpi implementation", m_DefaultPrefs.GetMpiName());

    if(useMPI) {
        m_Preferences->Put("mpiexec path", MPIExecPath);
    }

    return true;
}

//--------
// Update
//--------
// Update the GUI with the solver path values from the
// MITK database.
//
void sv4guiMPIPreferencePage::Update()
{
    m_Ui->checkBoxUseMPI->setChecked(m_Preferences->GetBool("use mpi", true));
    m_Ui->lineEditMPIExecPath->setText(m_Preferences->Get("mpiexec path",""));
}

