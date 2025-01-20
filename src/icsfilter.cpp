#include "icsfilter.h"
#include <QtGlobal>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
//#include <QFile>
//#include <QDir>
#include <QDate>
#include <QDateTime>
#include <QTimeZone>

/* example filter definition file
 * if no filter is defined, the input iCalendar-file is not modified
 * filter definitions are a JSON-file:
 * { "calendars": [
 *   { "label": "Haagan Karhut - https://haagankarhut.nimenhuuto.com/", // filter the ics-file, if mkcal->label = label
 *     "url": "https://haagankarhut.nimenhuuto.com/calendar/ical",
 * 	   "reminder": "120", // how many minutes before the start time - if not defined, a reminder is not set; overwrites the values in the ics-file
 *     "dayreminder": "18:00", // for full day events - if not defined, a reminder is not set for full day events; overwrites the values in the ics-file
 * 	   "bothReminders": "no", // no, yes: if both reminders are set, are both added for normal events - defaults to no
 *     // if no filter is defined, the component is not filtered out
 *     "filters": [ // only one item per component type - uses only one if multiple found
 *     	{ "component": "vevent",
 *     	  "action": "accept", // accept, reject: take in or drop out components that match this filter, defaults to reject
 *     	  "propMatches": 0.0, // 0 - 100, how many percent of the listed values need to match - defaults to zero
 *        "properties": [ // only one item per property - uses only one if multiple found
 *        { "property": "class",
 *          "type": "string", // string, number, date, day, time, defaults to string
 *          "valueMatches": 0.0, // 0 - 100, how many percent of the listed values need to match - defaults to zero (one match is always enough)
 *          "values": [
 *          { "criteria": "s", // =, !=, <>, <, >, <=, >=, s, !s (substring)
 *            "value": "ottelu"
 *          }]
 *        },
 *        { "property": "dtstart",
 *          "type": "time",
 *          "valueMatches": 100,
 *          "values": [
 *          { "criteria": "!=",
 *            "value": "20:30" // time = hh:mm, local time
 *          }]
 *        },
 *        { "property": "dtstart",
 *          "type": "day",
 *          "valueMatches": 100,
 *          "values": [
 *          { "criteria": "!=",
 *            "value": 2 // 1 - 7 = Monday - Sunday
 *          }]
 *        },
 *        { "property": "dtstart",
 *          "type": "date",
 *          "valueMatches": 100,
 *          "values": [
 *          { "criteria": ">",
 *            "value": "15.8." // date = dd.mm. or mm/dd,
 *          },
 *          { "criteria": "<",
 *            "value": "31.5."
 *          }]
 *        }]
 *      }]
 *    }]
 *  }
*/

icsFilter::icsFilter(QObject *parent) : QObject(parent)
{
    //QFile file;
    //int position;
    //position = settings.fileName().lastIndexOf("/");
    //filtersPath = settings.fileName().left(position);
    //filtersFileName = "iCalendarFilters.json";
    //file.setFileName(filtersPath+filtersFileName);
}

int icsFilter::addAlarm(int line0, int lineN, int reminderMins,
                        QTime reminderTime, bool onPreviousDay)
{
    // returns the number of added rows
    int i, result = 0;
    QString component, prName, prValue;
    QStringList paNames, paValues;
    QDate date;
    QTime time;
    QDateTime dateTime;

    // read start-time
    i = line0;
    while (i < lineN) {
        readProperty(modLines[i], prName, paNames, paValues, prValue);
        if (i == line0) { // "begin:vevent"
            component = prValue;
        } else if (prName.toUpper() == dtstart) {
            dateTime = propertyTime(prName, prValue, paNames, paValues, date, time);
            i = lineN;
        }
        i++;
    }

    // alarms can only be included into velement and vtodo
    if (isAlarmAllowed(component)) {
        if (time.isValid()) {
            result += addAlarmRelative(reminderMins, lineN);
        }
        if (!time.isValid() || bothReminders) {
            result += addAlarmAbsolute(reminderTime, onPreviousDay, date, lineN);
        }
    } else {
        qDebug() << "no alarms for" << component;
    }

    return result;
}

int icsFilter::addAlarmAbsolute(QTime time, bool onPreviousDay,
                                QDate date, int lineNr) {
    QDateTime trigger;
    QString alarmStr;
    //QTime time;
    int i0 = lineNr;

    if (!time.isValid()) {
        return 0;
    }

    if (onPreviousDay) {
        trigger.setDate(date.addDays(-1));
    } else {
        trigger.setDate(date);
    }
    trigger.setTime(time);
    trigger.setTimeSpec(Qt::LocalTime);

    alarmStr.append(trigger.toString("yyyyMMdd") + "T" + trigger.toString("HHmmss"));

    origLines.insert(lineNr, "BEGIN:VALARM");
    modLines.insert(lineNr, "BEGIN:VALARM");
    lineNr++;
    origLines.insert(lineNr, "TRIGGER;VALUE=DATE-TIME:" + alarmStr); // TRIGGER:-PT30M
    modLines.insert(lineNr, "TRIGGER;VALUE=DATE-TIME:" + alarmStr); // TRIGGER:-PT30M
    lineNr++;
    origLines.insert(lineNr, "ACTION:AUDIO");
    modLines.insert(lineNr, "ACTION:AUDIO");
    lineNr++;
    origLines.insert(lineNr, "END:VALARM");
    modLines.insert(lineNr, "END:VALARM");
    lineNr++;

    return lineNr - i0;
}

int icsFilter::addAlarmRelative(int min, int lineNr)
{
    QString advance = "PT";
    int i0 = lineNr;
    if (min > 0) {
        advance.insert(0, "-");
        advance.append(QString().setNum(min));
    } else {
        advance.append(QString().setNum(-min));
    }
    advance.append("M"); //minutes
    origLines.insert(lineNr, "BEGIN:VALARM");
    modLines.insert(lineNr, "BEGIN:VALARM");
    lineNr++;
    origLines.insert(lineNr, "TRIGGER:" + advance); // TRIGGER:-PT30M // '-' before
    modLines.insert(lineNr, "TRIGGER:" + advance); // TRIGGER:-PT30M
    lineNr++;
    origLines.insert(lineNr, "ACTION:AUDIO");
    modLines.insert(lineNr, "ACTION:AUDIO");
    lineNr++;
    origLines.insert(lineNr, "END:VALARM");
    modLines.insert(lineNr, "END:VALARM");
    lineNr++;

    return lineNr - i0;
}

// checks if             calendarName = filter."label".value or
//           ics."X-WR-CALNAME".value = filter."value".value
bool icsFilter::calendarFilterCheck(QJsonValue filterN,
        QString filterKey, QStringList properties, QStringList values)
{
    QJsonValue jsonVal;
    QJsonObject calObj;
    QString filteringLabel, filteringProperty, filteringValue;
    int j, jN;
    bool result;

    result = false;
    if (filterN.isObject()) {
        calObj = filterN.toObject();
        jsonVal = calObj.value(filterKey);
        if (filterKey.toLower() == keyName) {
            if (!jsonVal.isUndefined() && jsonVal.isString()) {
                filteringLabel = jsonVal.toString();
                if (filteringLabel.toLower() == calendarName.toLower()) {
                    result = true;
                }
            }
        } else {
            //jsonVal = calObj.value(filterKey);
            if (!jsonVal.isUndefined() && jsonVal.isString()) {
                filteringProperty = jsonVal.toString();
            } else {
                filteringProperty = keyRemoteName;
            }

            jsonVal = calObj.value(keyIdVal);
            if (!jsonVal.isUndefined() && jsonVal.isString()) {
                filteringValue = jsonVal.toString();
            }
            j = 0;
            jN = properties.length();
            while (j < jN) {
                if (properties.at(j).toLower() == filteringProperty.toLower()) {
                    if (values.at(j).toLower() == filteringValue.toLower()) {
                        result = true;
                        j = jN;
                    }
                }
                j++;
            }
        }
    } else {
        qDebug() << "Filter is not a jsonObject";
    }

    if(result) {
        if (filteringValue.isEmpty()) {
            filteringValue = calendarName;
        }
        qDebug() << "Found filter for calendar" << filteringValue;
    }

    return result;
}

// checks, if the value of the filterKey ("label" or "X-WR-CALNAME")
// fits the calendar name or the corresponding ics-calendar property
QJsonObject icsFilter::calendarFilterFind(QString filterKey,
        QStringList properties, QStringList values)
{
    QJsonValue jsonVal;
    QJsonArray jsonArr;
    QJsonObject result;
    int i, iN;

    jsonVal = filters.value(keyCalendars);
    if (jsonVal.isArray()) {
        jsonArr = jsonVal.toArray();
        i = 0;
        iN = jsonArr.count();

        while (i < iN) {
            jsonVal = jsonArr.at(i);
            if (calendarFilterCheck(jsonVal, filterKey, properties,
                                    values)) {
                result = jsonVal.toObject();
                i = iN;
            }
            i++;
        }
    } else {
        if (calendarFilterCheck(jsonVal, filterKey, properties, values)) {
            result = jsonVal.toObject();
        }
    }

    return result;
}

// checks, which filter to use for this calendar
QJsonObject icsFilter::calendarFilterGet(QStringList properties,
                                         QStringList values)
{
    QJsonObject result;

    //{ "calendars": [{
    //    "label": "haka", // which calendar to filter
    //    "idProperty": "X-WR-CALNAME", // which property to use if label is not defined
    //    "idValue": "www.nimenhuuto.com/haka", // value of "property"

    // find correct calendar by label
    // if not found, find by property and value
    result = calendarFilterFind(keyName, properties, values);
    if (result.isEmpty()) {
        result = calendarFilterFind(keyIdProperty, properties, values);
    }

    return result;
}

// reads the filter for the component
bool icsFilter::componentFilter(QString component,
            QJsonObject &cmpFilter, bool &isReject, float &matchPortion)
{
    QJsonValue jval;
    bool result, isOk;
    QString str;

    isOk = true;
    isReject = true; // default - "action": "reject"
    matchPortion = 0; // default - "propMatches": 0
    cmpFilter = listItem(cFilter, keyFilters, keyComponent, component);

    jval = cmpFilter.value(keyAction);
    if (jval.isString()) {
        if (jval.toString().toLower() == valueAccept) {
            isReject = false;
        }
    }

    jval = cmpFilter.value(keyPropMatches);
    if (jval.isDouble()) {
            matchPortion = jval.toDouble()/100;
    } else if (jval.isString()) {
        str = jval.toString();
        matchPortion = str.toFloat(&isOk)/100;
        if (!isOk) {
            qWarning() << "Not a number:" << keyPropMatches << ", " << str;
        }
    } else if (jval != QJsonValue::Undefined) {
        qWarning() << "Not a number:" << keyPropMatches;
    }

    if (cmpFilter.isEmpty()) {
        result = false;
    } else {
        result = true;
    }

    return result;
}

QString icsFilter::criteriaToString(filteringCriteria crit)
{
    //{NotDefined, Equal, NotEqual, EqualOrLarger, EqualOrSmaller,
    //  Larger, Smaller, SubString, NotSubString};
    QString result;
    if (crit == NotDefined) {
        result = "NotDefined";
    } else if (crit == Equal) {
        result = "Equal";
    } else if (crit == NotEqual) {
        result = "NotEqual";
    } else if (crit == EqualOrLarger) {
        result = "EqualOrLarger";
    } else if (crit == EqualOrSmaller) {
        result = "EqualOrSmaller";
    } else if (crit == Larger) {
        result = "Larger";
    } else if (crit == Smaller) {
        result = "Smaller";
    } else if (crit == SubString) {
        result = "SubString";
    } else if (crit == NotSubString) {
        result = "NotSubString";
    }

    return result;
}

// starts at lineNr
// returns the line number of the next 'end:vcalendar'
int icsFilter::filterCalendar(int lineNr)
{
    // a file may contain more than a single calendar, therefore notEndOfCal
    QRegExp beginCal, endCal, beginCmp, endCmp;
    QString component, prop, val;
    QStringList properties, values, params, parVals;
    QVector<QStringList> propParams, propParVals;
    QJsonValue jval;
    QTime reminderTime;
    int i, line0, lineNr2, nComponents, newRows, reminderMins;
    bool addReminder = false, isFilterSet, isOk, remindPreviousDay = true;

    nComponents = 0;
    //beginCal.setCaseSensitivity(Qt::CaseInsensitive);
    //beginCal.setPattern(("^begin:vcalendar$"));
    //endCal.setCaseSensitivity(Qt::CaseInsensitive);
    //endCal.setPattern(("^end:vcalendar$"));
    //beginCmp.setCaseSensitivity(Qt::CaseInsensitive);
    //endCmp.setCaseSensitivity(Qt::CaseInsensitive);

    // find "begin:calendar"
    component = vcalendar;
    lineCalBegin = findComponent(lineNr, component);
    lineCalEnd = findComponentEnd(lineCalBegin, component);
    if (lineCalEnd >= modLines.length()) {
        lineCalEnd = modLines.length() - 1;
    }
    lineNr = lineCalBegin + 1;
    if (lineNr >= lineCalEnd) {
        qWarning() << beginCal.pattern() << "not found";
        return lineNr;
    }

    line0 = lineNr; // "begin:vcalendar" on line lineNr-1;

    // read the calendar properties
    // calendarName = mClient->key("label") || property("X-WR-CALNAME")
    while (lineNr < lineCalEnd) {
        // read calendar properties, skip calendar components
        if (skipComponent(lineNr) == 0) {
            readProperty(modLines[lineNr], prop, params, parVals, val);
            properties.append(prop);
            values.append(val);
            propParams.append(params);
            propParVals.append(parVals);
        }
        lineNr++;
    }

    // read the filter for the calendar
    cFilter = calendarFilterGet(properties, values);
    isFilterSet = !cFilter.isEmpty();

    // read alarms
    //qDebug() << "read user defined alarms, filters" << isFilterSet;
    reminderMins = 0;
    if (isFilterSet) {
        jval = cFilter.value(keyReminder);
        if (!jval.isUndefined()) {
            if (jval.isString()) {
                reminderMins = jval.toString().toInt(&isOk);
                if (!isOk) {
                    qWarning() << "Converting reminder duration failed:" << jval.toString() << ". Using" << reminderMins << "minutes.";
                } else {
                    addReminder = true;
                    qDebug() << "Reminder for normal events" << reminderMins << "min.";
                }
            } else if (jval.isDouble()) {
                reminderMins = jval.toDouble();
                addReminder = true;
                qDebug() << "Reminder for normal events" << reminderMins << "min.";
            } else {
                qWarning() << "Converting reminder duration failed: the reminder is not a string nor a number.";
            }
        } else {
            qInfo() << "No reminder set for normal events.";
        }
        jval = cFilter.value(keyReminderDay);

        if (!jval.isUndefined()) {
            if (jval.isString()) {
                QString strTime = jval.toString();
                if (strTime.at(0) == '-') {
                    strTime = strTime.right(strTime.length()-1);
                    remindPreviousDay = false;
                }
                reminderTime = QTime::fromString(strTime, "h:mm");
                if (!reminderTime.isValid()) {
                    qWarning() << "Converting dayreminder time failed:" << jval.toString();
                } else {
                    qDebug() << "Reminder for full day events at" << reminderTime.toString("hh:mm");
                }
            } else {
                qWarning() << "Converting dayreminder time failed: the reminder is not a valid string.";
            }
        } else {
            qInfo() << "No reminder set for full day events.";
        }
        bothReminders = false;
        jval = cFilter.value(keyBothReminders);
        if (jval.isString() && jval.toString().toLower() == "yes") {
            bothReminders = true;
        }
    }

    // filter the events
    lineNr = line0;
    while (lineNr < lineCalEnd) {
        component.clear();
        lineNr = findComponent(lineNr, component);
        qDebug() << "start" << component;
        if (lineNr >= lineCalEnd) {
            if (nComponents < 1) {
                qDebug() << "No components found.";
            }
            return lineNr;
        } else {
            nComponents++;
        }
        beginCmp.setPattern("^BEGIN:" + component);
        if (beginCmp.indexIn(modLines[lineNr]) < 0) {
            qDebug() << "Row" << lineNr << "is NOT" << beginCmp.pattern();
        }
        if (isFilterSet) {
            lineNr2 = findComponentEnd(lineNr, component);
            endCmp.setPattern("^END:" + component);
            if (lineNr2 >= lineCalEnd) {
                lineNr2 = lineCalEnd;
                //notEndOfCal = false;
                qDebug() << "End of component" << nComponents << "-" <<
                            component << "-" << "from line" << lineNr
                         << "not found";
            } else if (endCmp.indexIn(modLines[lineNr2]) < 0) {
                qDebug() << "Row" << lineNr2 << "is NOT" << endCmp.pattern();
            }

            if (filterComponent(component, lineNr, lineNr2) < 0) { // filter rows out if < 0
                i = lineNr;
                while (i <= lineNr2 || modLines[i] == " ") { // modLines[i] == " " ???
                    modLines[i] = "";
                    i++;
                }
            } else {
                if (addReminder || reminderTime.isValid()) {
                    newRows = addAlarm(lineNr, lineNr2, reminderMins, reminderTime, remindPreviousDay);
                    lineNr2 += newRows;
                }
            }
            lineNr = lineNr2; // "end:vevent"
        }
        lineNr ++;
    }
    qDebug() << "Found" << nComponents << "components.";

    return lineNr;
}

// if result < 0, the lines should be filtered out
int icsFilter::filterComponent(QString component, int line0, int lineN)
{
    // modLines[line0] == "begin:vcomponent", modLines[lineN] == "end:vcomponent"
    int iProp, result, isMatch, matchSum, nrChecks;
    bool isFilter = false;
    bool isReject;
    float percentRequired;
    QStringList properties, values, paNames, paValues;
    QVector<QStringList> parameterNames, parameterValues;
    QString prName, prValue;
    QJsonObject cmpFilter;

    if (line0 < lineCalBegin || line0 >= lineCalEnd || lineN <= line0 || lineN >= lineCalEnd) {
        qDebug() << "line number (" << line0 << "," << lineN << ") too large ( >" << modLines.length() << ")";
        qInfo() << "component" << component << ", line" << line0 << "not checked";
        return 0;
    }

    // is a filter defined for the component
    isFilter = componentFilter(component, cmpFilter, isReject, percentRequired);

    iProp = 0;
    isMatch = 0;
    matchSum = 0;
    nrChecks = 0;
    result = 1;
    line0++;
    while (line0 < lineN) {
        readProperty(modLines[line0], prName, paNames, paValues, prValue);
        if (prName.toUpper() == "BEGIN") { // skip subcomponents
            line0 = findComponentEnd(line0, prValue) + 1;
        } else if (!prName.isEmpty()) {
            // check if the property matches the component filter
            iProp++;
            properties.append(prName);
            values.append(prValue);
            parameterNames.append(paNames);
            parameterValues.append(paValues);

            if (isFilter) {
                isMatch = isPropertyMatching(cmpFilter, prName, prValue, paNames, paValues);
                if (isMatch > 0) { // if isMatch == 0, prName is not used for filtering
                    matchSum += isMatch;
                }
                nrChecks++;
                if ((percentRequired == 0.0 && isMatch > 0) || (percentRequired == 1.0 && isMatch < 0)) {
                    line0 = lineN;
                }
            }
        }
        line0 ++;
    }

    // if a single match is enough, percentRequired = 0
    if (matchSum > 0 && matchSum >= percentRequired*nrChecks) {
        isMatch = 1;
    }
    // reject (result < 0) if
    // - isReject == true && a match is found
    // - isReject == false && a match is not found
    if ((isReject && isMatch > 0) || (!isReject && isMatch <= 0)) {
        result = -1;
    }

    return result;
}

QByteArray icsFilter::filterIcs(QString label, QByteArray origIcsData, QString filters)
{
    // label tells which filter to use. Exit if label is empty.
    // filters is a json-string. If it is empty, exit.
    // Copies origIcsData to origLines[] and modLines[], of which
    // modLines[] will be filtered. Lines to be filtered out will
    // be replaced by an empty line "". Unfolding will replace
    // the folded lines by " ".
    // resultIcs = origLines[i] + "\r\n", if modLines[i] != ""
    QByteArray resultIcs;
    QString icsFile(origIcsData);
    int iLine, emptyEnds;

    qDebug() << "Filtering calendar" << label;

    if (label.isEmpty()) {
        qWarning() << "Calendar label unspecified.";
        return origIcsData;
    }
    if (!readFilters(filters)) {
        qWarning() << "Reading filters unsuccessful. Ics-file not filtered." << filters.toUtf8();
        return origIcsData;
    }

    calendarName = label;

    // the lines should end with CRLF
    origLines = icsFile.split("\r\n");
    if (origLines.length() < 2) {
        origLines = icsFile.split("\n");
        qWarning() << "No proper end of lines. Number of inproper ones " << origLines.length();
    }
    modLines = origLines;

    qDebug() << "unfoldLines()";
    unfoldLines();

    // prevent some false error messages
    iLine = modLines.length() - 1;
    emptyEnds = 0;
    while (iLine > 0 && (modLines[iLine] == "" || modLines[iLine] == " ") ) {
        iLine--;
        emptyEnds++;
    }

    // check each calendar in the file
    qDebug() << "Rows in the ics file: " << modLines.length() << ".";
    iLine = 0;
    while (iLine >= 0 && iLine < modLines.length() - emptyEnds) {
        iLine = filterCalendar(iLine) + 1;
    }

    qDebug() << "Filtered, printing the result file.";
    // print the result, skip the filtered out = empty lines
    resultIcs = "";
    iLine = 0;
    while (iLine < modLines.length()) {
        if (modLines[iLine] != "") {
            resultIcs += origLines[iLine].toUtf8() + "\r\n";
        }
        iLine++;
    }

    return resultIcs;
}

icsFilter::filteringCriteria icsFilter::filterType(QJsonValue jVal,
                                                   propertyType vType)
{
    filteringCriteria result;
    QString cmp;

    result = NotDefined;
    if (jVal.isString()) {
        cmp = jVal.toString().toLower();
        if (cmp == "s") {
            result = SubString;
            if (vType != String) {
                qWarning() << "Criteria SubString defined for strings only.";
            }
        } else if (cmp == "!s") {
            result = NotSubString;
            if (vType != String) {
                qWarning() << "Criteria NotSubString defined for strings only.";
            }
        } else if (cmp == "=") {
            result = Equal;
        } else if (cmp == "!=" || cmp == "<>") {
            result = NotEqual;
        } else if (cmp == "<") {
            result = Smaller;
            if (vType == String) {
                qWarning() << "Criteria < not defined for strings.";
            }
        } else if (cmp == ">") {
            result = Larger;
            if (vType == String) {
                qWarning() << "Criteria > not defined for strings.";
            }
        } else if (cmp == "<=") {
            result = EqualOrSmaller;
            if (vType == String) {
                qWarning() << "Criteria <= not defined for strings.";
            }
        } else if (cmp == ">=") {
            result = EqualOrLarger;
            if (vType == String) {
                qWarning() << "Criteria >= not defined for strings.";
            }
        }
    }

    return result;
}

// searches for 'BEGIN:' or 'END:' +component starting from lineNr, and
// returns the line number of the first matching line
// if component is empty, searches for any vcomponent
// if component is not found, returns the numbers of lines in modLines
int icsFilter::findComponent(int lineNr,
                             QString &component, bool componentEnd)
{
    // searches for 'end:' if componentEnd == true
    bool search = true, undefinedComponent = false;
    QRegExp expression;

    if (component.isEmpty()) {
        component = "(V[A-Z]+)";
        undefinedComponent = true;
    }
    expression.setCaseSensitivity(Qt::CaseInsensitive);
    expression.setMinimal(false);
    if (componentEnd) {
        expression.setPattern("^END\\s*:\\s*" + component);
    } else {
        expression.setPattern("^BEGIN\\s*:\\s*" + component);
    }

    while (search && lineNr < modLines.length()) {
        if (expression.indexIn(modLines[lineNr]) >= 0) {
            search = false;
        } else {
            lineNr++;
        }
    }

    if (undefinedComponent && expression.captureCount() > 0) {
        component = expression.cap(expression.captureCount());
    }

    return lineNr;
}

int icsFilter::findComponentEnd(int lineNr, QString component)
{
    return findComponent(lineNr, component, true);
}

bool icsFilter::isAlarmAllowed(QString component)
{
    bool result = false;
    if (component.toUpper() == vevent ||
            component.toUpper() == vtodo) {
        result = true;
    }
    return result;
}

int icsFilter::isMatchingDate(QJsonValue jVal, filteringCriteria crit,
                              QString prop, QString value, QStringList parameters, QStringList parValues)
{
    QDate dateFilter, dateIcs;
    QDateTime dt;
    QTime timeIcs;
    int result, year;
    QRegExp dotDate, slashDate;

    dotDate.setCaseSensitivity(Qt::CaseInsensitive);
    slashDate.setCaseSensitivity(Qt::CaseInsensitive);
    dotDate.setPattern("^(\\d\\d?)\\.(\\d\\d?)[\\.]$");
    slashDate.setPattern("^(\\d?\\d)/(\\d?\\d)[/]$");

    year = QDate::currentDate().year();
    if (jVal.isString()) {
        if (dotDate.exactMatch(jVal.toString())) {
            dateFilter.setDate(year, dotDate.cap(2).toInt(), dotDate.cap(1).toInt());
        } else if (slashDate.exactMatch(jVal.toString())) {
            dateFilter.setDate(year, slashDate.cap(1).toInt(), slashDate.cap(2).toInt());
        }
    }
    if (!dateFilter.isValid()) {
        qWarning() << "Filter value is not a date:" << jVal.toString();
        return 0;
    }

    dt = propertyTime(prop, value, parameters, parValues, dateIcs, timeIcs);
    dateIcs = dt.date(); //propertyTime does not consider time zone when setting dateIcs

    if ( (crit == Equal && dateIcs == dateFilter) ||
         (crit == NotEqual && dateIcs != dateFilter) ||
         (crit == Larger && dateIcs > dateFilter) ||
         (crit == EqualOrLarger && dateIcs >= dateFilter) ||
         (crit == Smaller && dateIcs < dateFilter) ||
         (crit == EqualOrSmaller && dateIcs <= dateFilter)
         ) {
            result = matchSuccess;
    } else {
        result = matchFail;
    }

    return result;
}

// 1 - Monday ... 7 - Sunday
int icsFilter::isMatchingDay(QJsonValue jVal, filteringCriteria crit, QString prop, QString value, QStringList parameters, QStringList parValues)
{
    QDateTime dt;
    QDate dateIcs;
    QTime timeIcs;
    int dayFilter = 0, dayIcs, result;
    bool ok;

    if (jVal.isDouble()) {
        dayFilter = jVal.toInt();
    } else if (jVal.isString()) {
        dayFilter = jVal.toString().toInt(&ok); // 0 if not successfull
    }
    if (dayFilter < 1 || dayFilter > 7) {
        qWarning() << "Day of week not between 1 and 7.";
        return 0;
    }

    dt = propertyTime(prop, value, parameters, parValues, dateIcs, timeIcs);
    dayIcs = dateIcs.dayOfWeek();

    if ( (crit == Equal && dayIcs == dayFilter) ||
         (crit == NotEqual && dayIcs != dayFilter) ||
         (crit == Larger && dayIcs > dayFilter) ||
         (crit == EqualOrLarger && dayIcs >= dayFilter) ||
         (crit == Smaller && dayIcs < dayFilter) ||
         (crit == EqualOrSmaller && dayIcs <= dayFilter)
         ) {
            result = matchSuccess;
    } else {
        result = matchFail;
    }

    return result;
}

int icsFilter::isMatchingNumber(QJsonValue jVal, filteringCriteria crit,
                                QString prop, QString value)
{
    bool isOk;
    double dblFilter, dblProperty;
    int result;

    isOk = false;
    dblFilter = 0;
    if (jVal.isString()) {
        dblFilter = jVal.toString().toDouble(&isOk);
    } else if (jVal.isDouble()) {
        dblFilter = jVal.toDouble();
        isOk = true;
    }
    if(!isOk) {
        qWarning() << "Filter value is not a number:" << jVal.toString();
        return 0;
    }

    dblProperty = value.toDouble(&isOk);
    if (!isOk) {
        qWarning() << "Property value is not a number:" << prop << ":" << value;
        return 0;
    }

    if ( (crit == Equal && dblProperty == dblFilter) ||
         (crit == NotEqual && dblProperty != dblFilter) ||
         (crit == Larger && dblProperty > dblFilter) ||
         (crit == EqualOrLarger && dblProperty >= dblFilter) ||
         (crit == Smaller && dblProperty < dblFilter) ||
         (crit == EqualOrSmaller && dblProperty <= dblFilter)
         ) {
        result = matchSuccess;
    } else {
        result = matchFail;
    }
    return result;
}

int icsFilter::isMatchingString(QJsonValue jVal, filteringCriteria crit,
                                QString prop, QString value)
{
    int result = 0;
    QString filterValue;

    if (!jVal.isString()) {
        qWarning() << "For property " << prop << ", the filter value is not a string:" << jVal.toString();
        return 0;
    }

    filterValue = jVal.toString();
    if ( (crit == Equal && value.compare(filterValue, Qt::CaseInsensitive) == 0) ||
         (crit == NotEqual && value.compare(filterValue, Qt::CaseInsensitive) != 0) ||
         (crit == SubString && value.contains(filterValue, Qt::CaseInsensitive)) ||
         (crit == NotSubString && !value.contains(filterValue, Qt::CaseInsensitive))) {
        result = matchSuccess;
    } else {
        result = matchFail;
    }

    return result;
}

int icsFilter::isMatchingTime(QJsonValue jVal, filteringCriteria crit, QString prop, QString value, QStringList parameters, QStringList parValues)
{
    //DTSTART:19970714T133000 // Local time
    //DTSTART:19970714T173000Z // UTC time
    //DTSTART;TZID=America/New_York:19970714T133000 // Local time and time zone reference
    QTime timeFilter, timeProperty;
    QDate date;
    QDateTime dateTime;
    QString str;
    int result;

    if (jVal.isString()) {
        str = jVal.toString();
        timeFilter = QTime::fromString(str, "hh:mm");
    }
    if (!timeFilter.isValid()) {
        qWarning() << "Filter value is not a timevalue:" << jVal.toString();
        return 0;
    }
    dateTime = propertyTime(prop, value, parameters, parValues, date, timeProperty);
    timeProperty = dateTime.time();

    if ( (crit == Equal && timeProperty == timeFilter) ||
         (crit == NotEqual && timeProperty != timeFilter) ||
         (crit == Larger && timeProperty > timeFilter) ||
         (crit == EqualOrLarger && timeProperty >= timeFilter) ||
         (crit == Smaller && timeProperty < timeFilter) ||
         (crit == EqualOrSmaller && timeProperty <= timeFilter)
         ) {
        result = matchSuccess;
    } else {
        result = matchFail;
    }

    return result;
}

// does property.value in the iCalendar match cmpFilter
int icsFilter::isPropertyMatching(QJsonObject cmpFilter, QString property,
                                  QString value, QStringList parameters,
                                  QStringList parValues)
{
    // result < 0, if the current property does not match the filter
    // result > 0, if the current property matches the filter
    // result = 0, if no filters for the property were found
    int result = 0, match = 0, i, iN, nrChecks, matchSum;
    const int resultMatch = 1, resultNoMatch = -1, resultNotFound = 0;
    QJsonArray jarr;
    QJsonObject prFilter, jobj;
    QJsonValue jval;
    propertyType valType;
    filteringCriteria criteria = NotDefined;
    QString filterValue;//, cmp;
    bool stillAnd, filterFound, isOk;
    float percentRequired;

    result = resultNoMatch;
    // read the filters where properties[property] = property
    prFilter = listItem(cmpFilter, keyProperties, keyProperty, property);
    if (prFilter.isEmpty()) {
        return 0;
    }

    // type of the property value - string, number, date or time
    jval = prFilter.value(keyPropType);
    valType = String;
    if (jval.isString()) {
        filterValue = jval.toString().toLower();
        if (filterValue == "date") {
            valType = Date;
        } else if (filterValue == "number") {
            valType = Number;
        } else if (filterValue == "time") {
            valType = Time;
        } else if (filterValue == "day") {
            valType = Day;
        }
    }

    // for a match, how many conditions does the property need to match?
    // 0 = single, 100 = all
    jval = prFilter.value(keyValueMatches);
    if (jval.isDouble()) {
        percentRequired = jval.toDouble()/100;
    } else if (jval.isString()) {
        percentRequired = jval.toString().toFloat(&isOk)/100; // toFloat() returns 0.0 if conversion fails
        if (!isOk) {
            qWarning() << "Value of" << keyValueMatches << "is not a number:" << jval.toString();
        }
    } else {
        qWarning() << "Value of" << keyValueMatches << "is not a number or a string, but" << jval.type();
        percentRequired = 0;
    }
    if (percentRequired < 0 || percentRequired > 1) {
        qWarning() << "Amount of matches is not between 0 - 100 %: " << percentRequired*100;
    }

    // check which filters the property value matches
    stillAnd = true;
    filterFound = false;
    nrChecks = 0;
    matchSum = 0;
    jval = prFilter.value(keyValues); // jval = [ { "value": string, "criteria": string }]
    if (jval.isArray()) {
        jarr = jval.toArray();
        i = 0;
        iN = jarr.count();
        while (i < iN && stillAnd) {
            match = 0;
            jval = jarr.at(i); // { "value": "18:30", "criteria": ">" }
            if (jval.isObject()) {
                jobj = jval.toObject();
                jval = jobj.value(keyCriteria);
                criteria = filterType(jval, valType);

                jval = jobj.value(keyValue);
                if (valType == String) {
                    match = isMatchingString(jval, criteria, property, value);
                } else if (valType == Number) {
                    match = isMatchingNumber(jval, criteria, property, value);
                } else if (valType == Date) {
                    match = isMatchingDate(jval, criteria, property, value, parameters, parValues);
                } else if (valType == Day) {
                    match = isMatchingDay(jval, criteria, property, value, parameters, parValues);
                } else if (valType == Time) {
                    match = isMatchingTime(jval, criteria, property, value, parameters, parValues);
                }
                nrChecks++;
                // match == 0 : not checked, match == -1 : no match, match == 1 : match
                if (match > 0) {
                    matchSum += match;
                    if (percentRequired == 0) {
                        stillAnd = false;
                    }
                    filterFound = true;
                } else {
                    if (percentRequired == 1) {
                        stillAnd = false;
                    }
                    if (match < 0) {
                        filterFound = true;
                    }
                }
            }
            i++;
        }
    }

    if (matchSum > 0 && matchSum >= percentRequired*nrChecks) {
        result = resultMatch;
    } else if (!filterFound) {
        result = resultNotFound;
    }

    return result;
}

// returns the list item i where jObject.listName[i].key = value
QJsonObject icsFilter::listItem(QJsonObject jObject, QString listName,
                QString key, QString value, QString key2, QString value2)
{
    int i;
    QJsonObject result;
    i = listItemIndex(jObject, listName, key, value, key2, value2);
    if (i >= 0) {
        result = jObject.value(listName).toArray().at(i).toObject();
    } else if (i == -1) {
        result = jObject.value(listName).toObject();
    }
    return result;
}

// returns the index of the list item, where jObject.listName[i].key = value
int icsFilter::listItemIndex(QJsonObject jObject, QString listName, QString key, QString value, QString key2, QString value2)
{
    // returns -2 if listName[i].key != value
    // returns -1 if jObject.listName is not an array, but has key = value
    QJsonArray jArr;
    QJsonObject jObj;
    QJsonValue jVal, jVal2;
    int i, iN, result;
    bool isVal2Needed = key2.isEmpty();

    result = -2;
    jVal = jObject.value(listName);
    if (jVal.isArray()) {
        jArr = jVal.toArray();
        i = 0;
        iN = jArr.count();
        while (i < iN) {
            jVal = jArr.at(i);
            if (jVal.isObject()) {
                jObj = jVal.toObject();
                jVal = jObj.value(key);
                if (isVal2Needed) {
                    jVal2 = jObj.value(key2);
                }
                if (jVal.toString().toLower() == value.toLower() &&
                        (!isVal2Needed || jVal2.toString().toLower() == value2.toLower())) {
                    result = i;
                }

            }
            i++;
        }
    } else if (jVal.isObject()) {
        jObj = jVal.toObject();
        if (jObj.contains(key) && (!isVal2Needed || jObj.contains(key2))) {
            if (jObj.value(key).toString().toLower() == value.toLower() &&
                    (!isVal2Needed || jObj.value(key2).toString().toLower() == value2.toLower())) {
                result = -1;
            }
        }
    }

    return result;
}

//QString icsFilter::overWriteFiltersFile(QString jsonText)
//{
//    QFile fFile;
//    QTextStream fData;
//    QDir fDir;
    //QJsonDocument jsonFile;
    //QString fileContentssss;
    //jsonFile = QJsonDocument::fromJson(jsonText.toUtf8());
    //if (jsonFile.isNull()) {
    //    qWarning() << "JSON Parse error in string: > > > >\n" << jsonText << "\n< < < <";
    //}
    //fileContents = jsonFile.toJson(QJsonDocument::Indented);

//    fFile.setFileName(filtersPath + filtersFileName);
//    if (fFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
//        fData.setDevice(&fFile);
//        fData << jsonText;
//        fFile.close();
//    }
//
//    return fFile.errorString();
//}

// time in icalendar-file
QDateTime icsFilter::propertyTime(QString prop, QString timeStr,
                                  QStringList parameters,
                                  QStringList parValues,
                                  QDate &date, QTime &time)
{
    //date & time are in no time zone
    //DTSTART;VALUE=DATE:19970714 // day
    //DTSTART:19970714T133000 // Local time
    //DTSTART:19970714T173000Z // UTC time
    //;TZID=America/New_York:19970714T133000 // Local time and time zone reference
    QDateTime result;
    QTimeZone zone;
    QRegExp dtValue("(-?\\d+)[Tt](\\d\\d)(\\d\\d)(\\d\\d)"), dValue("(\\d+)");
    QString tmp, zoneName;
    bool isOk;
    int hr=-1, min=-1, sec=-1, yyyy=0, mm=-1, dd=-1, i;
    // time zone
    if (timeStr.at(timeStr.length() - 1) == 'Z') {
        QTimeZone utc(0);
        zone.swap(utc);
    } else {
        i = 0;
        while (i < parameters.length()) {
            if (parameters.at(i).toUpper() == "TZID") {
                zoneName = parValues.at(i);
                i = parameters.length();
                if (zone.isTimeZoneIdAvailable(zoneName.toUtf8())) {
                    QTimeZone zone2(zoneName.toUtf8());
                    zone.swap(zone2);
                } else {
                    qWarning() << "Time zone" << zoneName << "not available, using locale.\nAvailable time zones:" << '\n' << zone.availableTimeZoneIds();
                }
            }
            i++;
        }
    }
    // read time
    tmp = "";
    i = dtValue.indexIn(timeStr);
    if ( i >= 0) {
        hr = dtValue.cap(2).toInt(&isOk);
        if (isOk) {
            min = dtValue.cap(3).toInt(&isOk);
            if (isOk) {
                sec = dtValue.cap(4).toInt(&isOk);
            }
        }
        time.setHMS(hr, min, sec);
        tmp = dtValue.cap(1);
    } else {
        i = dValue.indexIn(timeStr);
        if (i >= 0) {
            tmp = dValue.cap(1);
            time.setHMS(-1,-1,-1);
        }
    }
    if (tmp.length() >= 5) {
        yyyy = tmp.leftRef(tmp.length() - 4).toInt();
        mm = tmp.midRef(tmp.length() - 4, 2).toInt();
        dd = tmp.rightRef(2).toInt();
    }
    date.setDate(yyyy, mm, dd);

    result.setDate(date);
    result.setTime(time);
    if (zone.isValid()) {
        result.setTimeZone(zone);
    } else {
        result.setTimeSpec(Qt::LocalTime);
    }

    if (!date.isValid()) {
        qWarning() << "Property value is not a timevalue:" << prop << " - " << timeStr << " - " << zoneName;
        qDebug() << "date-time?" << dtValue.capturedTexts() << "or date?" << dValue.capturedTexts();
    }

    return result.toLocalTime();
}

bool icsFilter::readFilters(QString filtersFileContents)
{
    // returns if fileContents.isEmpty()
    QJsonDocument json;
    QJsonParseError jsonError;
    const int filtersFileMinLength = 16; // {"calendars":[]}

    if (filtersFileContents.length() < filtersFileMinLength) {
        return false;
    }
    if (filtersFileContents.length() < filtersFileMinLength) {
        return false;
    }

    json = QJsonDocument::fromJson(filtersFileContents.toUtf8(), &jsonError);
    filters = json.object();
    if (json.isObject()) {
        qDebug() << "Reading filters successfull.";
    } else {
        qDebug() << "Reading filters unsuccessfull:\n";
        qDebug() << jsonError.errorString() << "\n at " << jsonError.offset;
    }

    return json.isObject();
}

// reads the property name, value and parameters
int icsFilter::readProperty(QString line, QString &name,
                            QStringList &pNames, QStringList &pValues,
                            QString &value)
{
    int i, j;

    i = readPropertyName(line, name);
    if (i <= 0) {
        if (line.length() > 1) { // folded lines equal " "
            qDebug() << "No property name found:" << line;
        }
    } else {
        j = readPropertyParameters(line, i, pNames, pValues);
        if (j < 0) {
            j = i;
        }
        value = readPropertyValue(line, j);
        if (value.length() < 1) {
            qInfo() << "No property value found:" << line;
        }
    }

    return 0;
}

// returns the length of the name
int icsFilter::readPropertyName(QString line, QString &name)
{
    QRegExp propName;

    propName.setCaseSensitivity(Qt::CaseInsensitive);
    propName.setMinimal(false);
    propName.setPattern("^[a-z0-9-]+");
    propName.indexIn(line);

    name = propName.cap();


    return name.length();
}

// returns the end position of the last parameter value
int icsFilter::readPropertyParameters(QString line, int position,
                                         QStringList &pNames,
                                         QStringList &pValues)
{
    QRegExp paramName, paramValue;
    QString values;
    int p1, p;

    p1 = position;
    p = position;
    pNames.clear();
    pValues.clear();
    paramName.setCaseSensitivity(Qt::CaseInsensitive);
    paramName.setMinimal(false);
    paramName.setPattern("^;([a-z0-9-]+)");
    // not quoted text must not contain '"', ";", ":", "," and controls except \t
    // quoted text must not contain '"' and controls except \t
    paramValue.setCaseSensitivity(Qt::CaseInsensitive);
    paramValue.setMinimal(false);
    paramValue.setPattern("^([^\";:,]+|\"[^\"]*\")");

    while (p > 0) {
        p = paramName.indexIn(line, p, QRegExp::CaretAtOffset);
        if (p >= 0) {
            pNames.append(paramName.cap(1));
            p += paramName.matchedLength();
            if (line.at(p) == '=') {
                p++;
            }
            p1 = p;
            while (p >= 0) {
                p = paramValue.indexIn(line, p, QRegExp::CaretAtOffset);
                p += paramValue.matchedLength();
                values.append(paramValue.cap(1));
                p1 = p;
                if (line.at(p) == ',') {
                    values.append(",");
                    p++;
                } else {
                    p = -1;
                }
            }
            pValues.append(values);
        }
    }

    return p1;
}

QString icsFilter::readPropertyValue(QString line, int position)
{
    QString result;
    int i;

    i = line.indexOf(":", position);
    if (i >= 0) {
        result = line.right(line.length() - i - 1);
    } else {
        result = "";
    }

    return result;
}

// returns the number of lines in the component including "begin:"&"end:"
// or 0, if lineNr is not "begin:"
int icsFilter::skipComponent(int &lineNr)
{
    QRegExp beginCmp, endCmp;//, beginSub, endSub;
    QString component, line;
    int iN, row0, result;

    beginCmp.setCaseSensitivity(Qt::CaseInsensitive);
    beginCmp.setPattern("^begin:");
    endCmp.setCaseSensitivity(Qt::CaseInsensitive);

    row0 = lineNr;
    iN = modLines.length();
    line = modLines[lineNr];
    if (beginCmp.indexIn(line) >= 0) {
        lineNr++;
        component = line.right(line.length() - beginCmp.pattern().length() + 1).toLower(); // '^'
        endCmp.setPattern("^end:" + component);
        while (lineNr < iN && endCmp.indexIn(modLines[lineNr]) < 0) {
            if (beginCmp.indexIn(modLines[lineNr]) >= 0) {
                skipComponent(lineNr);
            }
            lineNr++;
        }
    }

    if (lineNr == row0) {
        result = 0;
    } else {
        result = lineNr - row0 + 1;
    }
    return result;
}

/*
QString icsFilter::setFiltersFile(QString fileName, QString path)
{
    QString result;
    if (!path.isEmpty()) {
        filtersPath = path;
        if (filtersPath.at(filtersPath.length() - 1) != '/') {
            filtersPath += "/";
        }
    }
    if (!fileName.isEmpty()) {
        filtersFileName = fileName;
    }
    result.append(filtersPath);
    result.append(filtersFileName);

    return result;
}
// */

// unfolds the lines, replaces the folds by space " "
// returns the total number of unfolds
int icsFilter::unfoldLines()
{
    int i = 0, result = 0;
    while (i < modLines.length()) {
        result += unfoldLine(i);
        i++;
    }

    return result;
}

// unfolds line starting at lineNr, replaces the folds by space " "
// changes lineNr to the last fold line
// returns the number of unfolds
int icsFilter::unfoldLine(int &lineNr)
{
    QString nextLine, tmp;
    bool cont = true;
    int i = lineNr, result = 0;

    if (i >= modLines.length()) {
        return result;
    }

    tmp = modLines.at(i);
    while (i < modLines.length() - 2 && cont) {
        nextLine = modLines.at(i + 1);
        if (!nextLine.isEmpty() && (nextLine.at(0) == ' ' || nextLine.at(0) == '\t'))  {
            tmp.append(nextLine.right(nextLine.length() - 1));
            modLines.replace(lineNr, tmp);
            modLines.replace(i+1, " ");
            i++;
            result++;
        } else {
            cont = false;
        }
    }

    lineNr = i;

    return result;
}
