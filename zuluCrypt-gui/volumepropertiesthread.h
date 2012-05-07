#ifndef VOLUMEPROPERTIESTHREAD_H
#define VOLUMEPROPERTIESTHREAD_H

#include <QThreadPool>
#include <QRunnable>
#include "miscfunctions.h"
#include <QString>
#include <QStringList>
#include <sys/types.h>
#include <unistd.h>

#include "../zuluCrypt-cli/bin/bash_special_chars.h"

class volumePropertiesThread : public QObject, public QRunnable
{
	Q_OBJECT
public:
	explicit volumePropertiesThread(QString,QString);
	void start(void);
	~volumePropertiesThread();
signals:
	void finished(QString) ;
public slots:	
private:
	QString fuseblkGetFs(void) ;
	QString hashPath(QString)	;
	void run(void);
	QString m_fusefs ;
	QString m_path ;
	QString m_mpoint ;
	QString m_volumeProperties ;
};

#endif // VOLUMEPROPERTIESTHREAD_H
