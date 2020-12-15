/*
 File: ProcessDialog.cpp
 Created on: 07/12/2017
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Project
#include <ProcessDialog.h>
#include <Worker.h>

// C++
#include <iostream>

// Qt
#include <QEvent>
#include <QKeyEvent>
#include <QtWinExtras/QWinTaskbarProgress>

//--------------------------------------------------------------------
ProcessDialog::ProcessDialog(const std::vector<std::filesystem::path> &files, const Utils::TranscoderConfiguration& config, QWidget* parent, Qt::WindowFlags flags)
: QDialog        (parent, flags)
, m_files        (files)
, m_num_workers  {0}
, m_configuration{config}
, m_errorsCount  {0}
, m_finished_transcoding{false}
, m_taskBarButton{nullptr}
{
  setupUi(this);

  register_av_lock_manager();

  connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(stop()));
  connect(m_clipboard,    SIGNAL(pressed()), this, SLOT(onClipboardPressed()));

  setWindowFlags(windowFlags() & ~(Qt::WindowContextHelpButtonHint) & Qt::WindowMaximizeButtonHint);

  m_log->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

  const auto max_workers = m_configuration.numberOfThreads();
  int total_jobs = m_files.size();

  m_globalProgress->setMinimum(0);
  m_globalProgress->setMaximum(total_jobs);

  auto boxLayout = new QVBoxLayout();
  m_workers->setLayout(boxLayout);

  auto initial_jobs = std::min(total_jobs, max_workers);

  for(int i = 0; i < initial_jobs; ++i)
  {
    auto bar = new QProgressBar();
    bar->setAlignment(Qt::AlignCenter);
    bar->setMaximum(0);
    bar->setMaximum(100);
    bar->setValue(0);
    bar->setEnabled(false);

    m_progress_bars[bar] = nullptr;

    boxLayout->addWidget(bar);
  }

  m_finished_transcoding = (files.size() == 0);

  create_threads();
}

//--------------------------------------------------------------------
ProcessDialog::~ProcessDialog()
{
  m_progress_bars.clear();
  m_taskBarButton->deleteLater();
  unregister_av_lock_manager();
}

//-----------------------------------------------------------------
void ProcessDialog::closeEvent(QCloseEvent *e)
{
  stop();

  for(auto worker: m_progress_bars.values())
  {
    delete worker;
  }

  m_progress_bars.clear();

  QDialog::closeEvent(e);
}

//-----------------------------------------------------------------
void ProcessDialog::log_error(const QString &message)
{
  QMutexLocker lock(&m_mutex);

  ++m_errorsCount;
  m_log->setTextColor(Qt::red);
  m_log->append(QString("ERROR: ") + message);

  m_errorsLabel->setStyleSheet("QLabel { color: rgb(255, 0, 0); };");
  m_errorsCountLabel->setStyleSheet("QLabel { color: rgb(255, 0, 0); };");
  m_errorsCountLabel->setText(QString().number(m_errorsCount));
}

//-----------------------------------------------------------------
void ProcessDialog::log_information(const QString &message)
{
  QMutexLocker lock(&m_mutex);
  m_log->setTextColor(Qt::black);
  m_log->append(message);
}

//-----------------------------------------------------------------
void ProcessDialog::stop()
{
  disconnect(m_cancelButton, SIGNAL(clicked()), this, SLOT(stop()));

  for(auto worker: m_progress_bars.values())
  {
    if(worker != nullptr)
    {
      worker->stop();
      worker->wait();
    }
  }

  m_cancelButton->setText(tr("Exit"));
  connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(close()));
}

//-----------------------------------------------------------------
void ProcessDialog::increment_global_progress()
{
  m_mutex.lock();

  auto worker = qobject_cast<Worker *>(sender());
  Q_ASSERT(worker);

  disconnect(worker, SIGNAL(error_message(const QString &)),       this, SLOT(log_error(const QString &)));
  disconnect(worker, SIGNAL(information_message(const QString &)), this, SLOT(log_information(const QString &)));
  disconnect(worker, SIGNAL(finished()),                           this, SLOT(increment_global_progress()));

  if(!worker->has_been_cancelled())
  {
    auto value = m_globalProgress->value();
    m_globalProgress->setValue(++value);
    m_taskBarButton->progress()->setValue(value);
  }

  --m_num_workers;

  auto bar = m_progress_bars.key(worker);
  Q_ASSERT(bar);

  disconnect(worker, SIGNAL(progress(int)),
             bar,       SLOT(setValue(int)));

  m_progress_bars[bar] = nullptr;
  bar->setEnabled(false);
  bar->setFormat("Idle");

  auto cancelled = worker->has_been_cancelled();
  delete worker;

  if((m_globalProgress->maximum() == m_globalProgress->value()) || cancelled)
  {
    disconnect(m_cancelButton, SIGNAL(clicked()),
               this,           SLOT(stop()));

    connect(m_cancelButton,    SIGNAL(clicked()),
            this,              SLOT(exit_dialog()));

    m_cancelButton->setText("Exit");
    m_clipboard->setEnabled(true);
  }

  if(m_files.empty() && m_num_workers == 0)
  {
    m_finished_transcoding = true;
  }

  m_mutex.unlock();

  if(!cancelled && !m_finished_transcoding)
  {
    create_threads();
  }
}

//-----------------------------------------------------------------
void ProcessDialog::create_threads()
{
  QMutexLocker lock(&m_mutex);

  while(m_num_workers < m_configuration.numberOfThreads() && m_files.size() > 0)
  {
    create_transcoder();
  }
}

//-----------------------------------------------------------------
void ProcessDialog::create_transcoder()
{
  const std::filesystem::path filename = m_files.front();
  m_files.erase(m_files.cbegin());

  ++m_num_workers;

  auto worker = new Worker(filename, m_configuration);

  auto message = QString::fromStdWString(filename.filename().wstring());
  assign_bar_to_worker(worker, message);

  worker->start();
}

//-----------------------------------------------------------------
void ProcessDialog::assign_bar_to_worker(Worker* worker, const QString& message)
{
  connect(worker, SIGNAL(error_message(const QString &)),       this, SLOT(log_error(const QString &)));
  connect(worker, SIGNAL(information_message(const QString &)), this, SLOT(log_information(const QString &)));
  connect(worker, SIGNAL(finished()),                           this, SLOT(increment_global_progress()));

  for(auto bar: m_progress_bars.keys())
  {
    if(m_progress_bars[bar] == nullptr)
    {
      m_progress_bars[bar] = worker;
      bar->setValue(0);
      bar->setEnabled(true);
      bar->setFormat(message);

      connect(worker, SIGNAL(progress(int)), bar, SLOT(setValue(int)));

      break;
    }
  }
}

//-----------------------------------------------------------------
void ProcessDialog::onClipboardPressed() const
{
  m_log->selectAll();
  m_log->copy();
}

//-----------------------------------------------------------------
bool ProcessDialog::event(QEvent *e)
{
  if(e->type() == QEvent::KeyPress)
  {
    auto ke = dynamic_cast<QKeyEvent *>(e);
    if(ke && (ke->key() == Qt::Key_Escape || ke->key() == Qt::Key_Enter))
    {
      e->accept();
      close();
      return true;
    }
  }

  return QDialog::event(e);
}

//-----------------------------------------------------------------
int ProcessDialog::lock_manager(void **mutex, AVLockOp operation)
{
  QMutex *passed_mutex;
  switch (operation)
  {
    case AV_LOCK_CREATE:
      *mutex = new QMutex();
      return 0;
    case AV_LOCK_OBTAIN:
      passed_mutex = reinterpret_cast<QMutex *>(*mutex);
      passed_mutex->lock();
      return 0;
    case AV_LOCK_RELEASE:
      passed_mutex = reinterpret_cast<QMutex *>(*mutex);
      passed_mutex->unlock();
      return 0;
    case AV_LOCK_DESTROY:
      passed_mutex = reinterpret_cast<QMutex *>(*mutex);
      delete passed_mutex;
      return 0;
  }
  return 1;
}

//-----------------------------------------------------------------
void ProcessDialog::register_av_lock_manager()
{
  av_lockmgr_register(ProcessDialog::lock_manager);
}

//-----------------------------------------------------------------
void ProcessDialog::unregister_av_lock_manager()
{
  av_lockmgr_register(nullptr);
}

//-----------------------------------------------------------------
void ProcessDialog::exit_dialog()
{
  close();
}

//-----------------------------------------------------------------
void ProcessDialog::showEvent(QShowEvent *e)
{
  QDialog::showEvent(e);

  m_taskBarButton = new QWinTaskbarButton(this);
  m_taskBarButton->setWindow(this->windowHandle());
  m_taskBarButton->progress()->setRange(m_globalProgress->minimum(), m_globalProgress->maximum());
  m_taskBarButton->progress()->setVisible(true);
  m_taskBarButton->progress()->setValue(0);
}
