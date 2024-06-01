#ifndef ICSFILTER_H
#define ICSFILTER_H

#include <QObject>
#include <QJsonObject>

class icsFilter : public QObject
{
    Q_OBJECT
public:
    explicit icsFilter(QObject *parent = nullptr);
    Q_INVOKABLE QByteArray filterIcs(QString label, QByteArray origIcsData, QString filters = "");
    Q_INVOKABLE QString overWriteFiltersFile(QString jsonText);
    Q_INVOKABLE QString readFiltersFile(QString fileName = "", QString path = "");
    Q_INVOKABLE QString setFiltersFile(QString fileName = "", QString path = "");

private:
    QJsonObject filters, cFilter, newFilter;
    QString calendarName, filtersFileName, filtersPath;
    QStringList modLines, origLines;
    int alarmAdvance, alarmTime;
    const QString keyCalendars = "calendars", keyRemoteName = "X-WR-CALNAME",
        keyName = "label", keyIdProperty = "idProperty",
        keyIdVal = "idValue", keyReminder = "reminder",
        keyReminderDay = "dayreminder", keyFilters = "filters",
        keyBothReminders = "bothReminders";
    const QString keyComponent = "component", keyAction = "action",
        keyPropMatches = "propMatches", keyProperties = "properties";
    const QString keyProperty = "property", keyPropType = "type",
        keyValueMatches = "valueMatches", keyValues = "values";
    const QString keyValue = "value", keyCriteria = "criteria";
    const QString vevent = "vevent", vtodo = "vtodo", vjournal = "vjournal",
        vfreebusy = "vfreebusy", vtimezone = "vtimezone", valarm = "valarm";
    const QString dtstart = "dtstart", valueAccept = "accept", valueReject = "reject";
    enum filteringCriteria {NotDefined, Equal, NotEqual, EqualOrLarger, EqualOrSmaller, Larger, Smaller, SubString, NotSubString};
    enum propertyType {Date, Day, Number, String, Time};
    const int matchFail = -1, matchSuccess = 1;

    int addAlarm(int lineNr, int lineN, int reminderMins, QTime reminderTime, bool onPreviousDay);
    int addAlarmRelative(int min, int lineNr);
    int addAlarmAbsolute(QTime time, bool onPreviousDay, QDate date, int lineNr);
    bool bothReminders = true;
    bool calendarFilterCheck(QJsonValue filterN, QString filterKey, QStringList properties, QStringList values);
    QJsonObject calendarFilterFind(QString filterKey, QStringList properties, QStringList values);
    QJsonObject calendarFilterGet(QStringList properties, QStringList values);
    bool componentFilter(QString component, QJsonObject &cmpFilter, bool &isReject, float &percentMatches);
    bool componentFilteringType(QString component);
    QString criteriaToString(filteringCriteria crit);
    int filterCalendar(int lineNr); // QStringList &lines,
    int filterComponent(QString component, int line0, int lineN);
    filteringCriteria filterType(QJsonValue jVal, propertyType vType);
    int findComponent(int lineNr, QString &component, bool componentEnd=false);
    int findComponentEnd(int lineNr, QString component);
    bool isAlarmAllowed(QString component);
    bool isCalendarMatching(QStringList properties, QStringList values);
    int isMatchingDate(QJsonValue jVal, filteringCriteria crit, QString prop, QString value, QStringList parameters, QStringList parValues);
    int isMatchingDay(QJsonValue jVal, filteringCriteria crit, QString prop, QString value, QStringList parameters, QStringList parValues);
    int isMatchingNumber(QJsonValue jVal, filteringCriteria crit, QString prop, QString value);
    int isMatchingString(QJsonValue jVal, filteringCriteria crit, QString prop, QString value);
    int isMatchingTime(QJsonValue jVal, filteringCriteria crit, QString prop, QString value, QStringList parameters, QStringList parValues);
    int isPropertyMatching(QJsonObject cmpFilter, QString property, QString value, QStringList parameters, QStringList parValues);
    QJsonObject listItem(QJsonObject jObject, QString listName, QString key, QString value, QString key2 = "", QString value2 = "");
    int listItemIndex(QJsonObject jObject, QString listName, QString key, QString value, QString key2 = "", QString value2 = "");
    QJsonObject propertyFilter(QJsonObject cmpFilter, QString property);
    filteringCriteria propertyFilterType(QString calendar, QString component, QString property);
    QDateTime propertyTime(QString prop, QString timeStr, QStringList parameters, QStringList parValues, QDate &date, QTime &time);
    QString readCalendarFilters(QString label);
    QString readCalendarFilters(QString property, QString value);
    bool readFilters(QString filtersFileContents);
    int readProperty(QString line, QString &name, QStringList &pNames, QStringList &pValues, QString &value);
    int readPropertyName(QString line, QString &name);
    int readPropertyParameters(QString line, int position, QStringList &pNames, QStringList &pValues);
    QString readPropertyValue(QString line, int position);
    int skipComponent(int &lineNr);
    int unfoldLine(int &lineNr);
    int unfoldLines();

};

#endif // ICSFILTER_H
