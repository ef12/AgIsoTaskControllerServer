/****************************************************************************
** Meta object code from reading C++ file 'TcBridge.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/gui/TcBridge.hpp"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TcBridge.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS = QtMocHelpers::stringData(
    "agisotc::TcBridge",
    "runningChanged",
    "",
    "taskActiveChanged",
    "selectedClientChanged",
    "sectionDdiChanged",
    "sectionCountChanged",
    "sectionStatesChanged",
    "statusTextChanged",
    "gpsChanged",
    "trackChanged",
    "fieldsChanged",
    "tasksChanged",
    "implementChanged",
    "implementDdisChanged",
    "autoDdiSyncChanged",
    "boundaryChanged",
    "workChanged",
    "drivingControlsChanged",
    "identifyBanner",
    "tcNumber",
    "startServer",
    "driver",
    "channel",
    "booms",
    "sections",
    "channels",
    "stopServer",
    "poll",
    "selectClient",
    "address",
    "setTaskActive",
    "active",
    "requestValue",
    "ddi",
    "element",
    "setValue",
    "value",
    "acknowledge",
    "sendMeasurement",
    "kind",
    "setSectionDdi",
    "setSectionCount",
    "count",
    "loadPoolFile",
    "fileUrl",
    "clearPool",
    "clearLog",
    "startGps",
    "source",
    "serialPort",
    "baudRate",
    "latitude",
    "longitude",
    "stopGps",
    "setSimulationMotion",
    "speedKph",
    "courseDeg",
    "nudgeSimulation",
    "forwardMeters",
    "turnDegrees",
    "createField",
    "name",
    "widthM",
    "lengthM",
    "selectField",
    "index",
    "createTask",
    "selectTask",
    "startSelectedTask",
    "pauseSelectedTask",
    "stopSelectedTask",
    "clearTrack",
    "setAutoDdiSync",
    "enabled",
    "setDdiSyncIntervalMs",
    "intervalMs",
    "requestImplementDdis",
    "startBoundaryRecording",
    "finishBoundaryRecording",
    "cancelBoundaryRecording",
    "setSteeringAngle",
    "degrees",
    "setThrottleKph",
    "adjustThrottle",
    "deltaKph",
    "stopTractor",
    "clearWorkedArea",
    "running",
    "taskActive",
    "selectedClient",
    "sectionDdi",
    "sectionCount",
    "sectionStates",
    "statusText",
    "gpsRunning",
    "gpsValid",
    "gpsSourceText",
    "gpsLatitude",
    "gpsLongitude",
    "gpsSpeedKph",
    "gpsCourse",
    "tractorX",
    "tractorZ",
    "trackPoints",
    "workedPoints",
    "fieldBoundaryPoints",
    "boundaryRecording",
    "boundaryPointCount",
    "fieldNames",
    "taskNames",
    "selectedFieldIndex",
    "selectedTaskIndex",
    "activeFieldName",
    "activeTaskName",
    "fieldWidthM",
    "fieldLengthM",
    "implementName",
    "implementGeometryStatus",
    "implementElements",
    "implementDdis",
    "autoDdiSync",
    "ddiSyncIntervalMs",
    "tcBasicData",
    "activeSectionCount",
    "workedAreaHa",
    "workedDistanceM",
    "workedTimeSeconds",
    "steeringAngle",
    "throttleKph",
    "implementX",
    "implementZ",
    "implementCourse",
    "clientModel",
    "ClientListModel*",
    "ddopModel",
    "DdopModel*",
    "valueModel",
    "ProcessDataModel*",
    "logModel",
    "LogModel*",
    "MeasurementKind",
    "TimeInterval",
    "DistanceInterval",
    "MinimumThreshold",
    "MaximumThreshold",
    "ChangeThreshold"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS_t {
    uint offsetsAndSizes[294];
    char stringdata0[18];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[18];
    char stringdata4[22];
    char stringdata5[18];
    char stringdata6[20];
    char stringdata7[21];
    char stringdata8[18];
    char stringdata9[11];
    char stringdata10[13];
    char stringdata11[14];
    char stringdata12[13];
    char stringdata13[17];
    char stringdata14[21];
    char stringdata15[19];
    char stringdata16[16];
    char stringdata17[12];
    char stringdata18[23];
    char stringdata19[15];
    char stringdata20[9];
    char stringdata21[12];
    char stringdata22[7];
    char stringdata23[8];
    char stringdata24[6];
    char stringdata25[9];
    char stringdata26[9];
    char stringdata27[11];
    char stringdata28[5];
    char stringdata29[13];
    char stringdata30[8];
    char stringdata31[14];
    char stringdata32[7];
    char stringdata33[13];
    char stringdata34[4];
    char stringdata35[8];
    char stringdata36[9];
    char stringdata37[6];
    char stringdata38[12];
    char stringdata39[16];
    char stringdata40[5];
    char stringdata41[14];
    char stringdata42[16];
    char stringdata43[6];
    char stringdata44[13];
    char stringdata45[8];
    char stringdata46[10];
    char stringdata47[9];
    char stringdata48[9];
    char stringdata49[7];
    char stringdata50[11];
    char stringdata51[9];
    char stringdata52[9];
    char stringdata53[10];
    char stringdata54[8];
    char stringdata55[20];
    char stringdata56[9];
    char stringdata57[10];
    char stringdata58[16];
    char stringdata59[14];
    char stringdata60[12];
    char stringdata61[12];
    char stringdata62[5];
    char stringdata63[7];
    char stringdata64[8];
    char stringdata65[12];
    char stringdata66[6];
    char stringdata67[11];
    char stringdata68[11];
    char stringdata69[18];
    char stringdata70[18];
    char stringdata71[17];
    char stringdata72[11];
    char stringdata73[15];
    char stringdata74[8];
    char stringdata75[21];
    char stringdata76[11];
    char stringdata77[21];
    char stringdata78[23];
    char stringdata79[24];
    char stringdata80[24];
    char stringdata81[17];
    char stringdata82[8];
    char stringdata83[15];
    char stringdata84[15];
    char stringdata85[9];
    char stringdata86[12];
    char stringdata87[16];
    char stringdata88[8];
    char stringdata89[11];
    char stringdata90[15];
    char stringdata91[11];
    char stringdata92[13];
    char stringdata93[14];
    char stringdata94[11];
    char stringdata95[11];
    char stringdata96[9];
    char stringdata97[14];
    char stringdata98[12];
    char stringdata99[13];
    char stringdata100[12];
    char stringdata101[10];
    char stringdata102[9];
    char stringdata103[9];
    char stringdata104[12];
    char stringdata105[13];
    char stringdata106[20];
    char stringdata107[18];
    char stringdata108[19];
    char stringdata109[11];
    char stringdata110[10];
    char stringdata111[19];
    char stringdata112[18];
    char stringdata113[16];
    char stringdata114[15];
    char stringdata115[12];
    char stringdata116[13];
    char stringdata117[14];
    char stringdata118[24];
    char stringdata119[18];
    char stringdata120[14];
    char stringdata121[12];
    char stringdata122[18];
    char stringdata123[12];
    char stringdata124[19];
    char stringdata125[13];
    char stringdata126[16];
    char stringdata127[18];
    char stringdata128[14];
    char stringdata129[12];
    char stringdata130[11];
    char stringdata131[11];
    char stringdata132[16];
    char stringdata133[12];
    char stringdata134[17];
    char stringdata135[10];
    char stringdata136[11];
    char stringdata137[11];
    char stringdata138[18];
    char stringdata139[9];
    char stringdata140[10];
    char stringdata141[16];
    char stringdata142[13];
    char stringdata143[17];
    char stringdata144[17];
    char stringdata145[17];
    char stringdata146[16];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS_t qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS = {
    {
        QT_MOC_LITERAL(0, 17),  // "agisotc::TcBridge"
        QT_MOC_LITERAL(18, 14),  // "runningChanged"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 17),  // "taskActiveChanged"
        QT_MOC_LITERAL(52, 21),  // "selectedClientChanged"
        QT_MOC_LITERAL(74, 17),  // "sectionDdiChanged"
        QT_MOC_LITERAL(92, 19),  // "sectionCountChanged"
        QT_MOC_LITERAL(112, 20),  // "sectionStatesChanged"
        QT_MOC_LITERAL(133, 17),  // "statusTextChanged"
        QT_MOC_LITERAL(151, 10),  // "gpsChanged"
        QT_MOC_LITERAL(162, 12),  // "trackChanged"
        QT_MOC_LITERAL(175, 13),  // "fieldsChanged"
        QT_MOC_LITERAL(189, 12),  // "tasksChanged"
        QT_MOC_LITERAL(202, 16),  // "implementChanged"
        QT_MOC_LITERAL(219, 20),  // "implementDdisChanged"
        QT_MOC_LITERAL(240, 18),  // "autoDdiSyncChanged"
        QT_MOC_LITERAL(259, 15),  // "boundaryChanged"
        QT_MOC_LITERAL(275, 11),  // "workChanged"
        QT_MOC_LITERAL(287, 22),  // "drivingControlsChanged"
        QT_MOC_LITERAL(310, 14),  // "identifyBanner"
        QT_MOC_LITERAL(325, 8),  // "tcNumber"
        QT_MOC_LITERAL(334, 11),  // "startServer"
        QT_MOC_LITERAL(346, 6),  // "driver"
        QT_MOC_LITERAL(353, 7),  // "channel"
        QT_MOC_LITERAL(361, 5),  // "booms"
        QT_MOC_LITERAL(367, 8),  // "sections"
        QT_MOC_LITERAL(376, 8),  // "channels"
        QT_MOC_LITERAL(385, 10),  // "stopServer"
        QT_MOC_LITERAL(396, 4),  // "poll"
        QT_MOC_LITERAL(401, 12),  // "selectClient"
        QT_MOC_LITERAL(414, 7),  // "address"
        QT_MOC_LITERAL(422, 13),  // "setTaskActive"
        QT_MOC_LITERAL(436, 6),  // "active"
        QT_MOC_LITERAL(443, 12),  // "requestValue"
        QT_MOC_LITERAL(456, 3),  // "ddi"
        QT_MOC_LITERAL(460, 7),  // "element"
        QT_MOC_LITERAL(468, 8),  // "setValue"
        QT_MOC_LITERAL(477, 5),  // "value"
        QT_MOC_LITERAL(483, 11),  // "acknowledge"
        QT_MOC_LITERAL(495, 15),  // "sendMeasurement"
        QT_MOC_LITERAL(511, 4),  // "kind"
        QT_MOC_LITERAL(516, 13),  // "setSectionDdi"
        QT_MOC_LITERAL(530, 15),  // "setSectionCount"
        QT_MOC_LITERAL(546, 5),  // "count"
        QT_MOC_LITERAL(552, 12),  // "loadPoolFile"
        QT_MOC_LITERAL(565, 7),  // "fileUrl"
        QT_MOC_LITERAL(573, 9),  // "clearPool"
        QT_MOC_LITERAL(583, 8),  // "clearLog"
        QT_MOC_LITERAL(592, 8),  // "startGps"
        QT_MOC_LITERAL(601, 6),  // "source"
        QT_MOC_LITERAL(608, 10),  // "serialPort"
        QT_MOC_LITERAL(619, 8),  // "baudRate"
        QT_MOC_LITERAL(628, 8),  // "latitude"
        QT_MOC_LITERAL(637, 9),  // "longitude"
        QT_MOC_LITERAL(647, 7),  // "stopGps"
        QT_MOC_LITERAL(655, 19),  // "setSimulationMotion"
        QT_MOC_LITERAL(675, 8),  // "speedKph"
        QT_MOC_LITERAL(684, 9),  // "courseDeg"
        QT_MOC_LITERAL(694, 15),  // "nudgeSimulation"
        QT_MOC_LITERAL(710, 13),  // "forwardMeters"
        QT_MOC_LITERAL(724, 11),  // "turnDegrees"
        QT_MOC_LITERAL(736, 11),  // "createField"
        QT_MOC_LITERAL(748, 4),  // "name"
        QT_MOC_LITERAL(753, 6),  // "widthM"
        QT_MOC_LITERAL(760, 7),  // "lengthM"
        QT_MOC_LITERAL(768, 11),  // "selectField"
        QT_MOC_LITERAL(780, 5),  // "index"
        QT_MOC_LITERAL(786, 10),  // "createTask"
        QT_MOC_LITERAL(797, 10),  // "selectTask"
        QT_MOC_LITERAL(808, 17),  // "startSelectedTask"
        QT_MOC_LITERAL(826, 17),  // "pauseSelectedTask"
        QT_MOC_LITERAL(844, 16),  // "stopSelectedTask"
        QT_MOC_LITERAL(861, 10),  // "clearTrack"
        QT_MOC_LITERAL(872, 14),  // "setAutoDdiSync"
        QT_MOC_LITERAL(887, 7),  // "enabled"
        QT_MOC_LITERAL(895, 20),  // "setDdiSyncIntervalMs"
        QT_MOC_LITERAL(916, 10),  // "intervalMs"
        QT_MOC_LITERAL(927, 20),  // "requestImplementDdis"
        QT_MOC_LITERAL(948, 22),  // "startBoundaryRecording"
        QT_MOC_LITERAL(971, 23),  // "finishBoundaryRecording"
        QT_MOC_LITERAL(995, 23),  // "cancelBoundaryRecording"
        QT_MOC_LITERAL(1019, 16),  // "setSteeringAngle"
        QT_MOC_LITERAL(1036, 7),  // "degrees"
        QT_MOC_LITERAL(1044, 14),  // "setThrottleKph"
        QT_MOC_LITERAL(1059, 14),  // "adjustThrottle"
        QT_MOC_LITERAL(1074, 8),  // "deltaKph"
        QT_MOC_LITERAL(1083, 11),  // "stopTractor"
        QT_MOC_LITERAL(1095, 15),  // "clearWorkedArea"
        QT_MOC_LITERAL(1111, 7),  // "running"
        QT_MOC_LITERAL(1119, 10),  // "taskActive"
        QT_MOC_LITERAL(1130, 14),  // "selectedClient"
        QT_MOC_LITERAL(1145, 10),  // "sectionDdi"
        QT_MOC_LITERAL(1156, 12),  // "sectionCount"
        QT_MOC_LITERAL(1169, 13),  // "sectionStates"
        QT_MOC_LITERAL(1183, 10),  // "statusText"
        QT_MOC_LITERAL(1194, 10),  // "gpsRunning"
        QT_MOC_LITERAL(1205, 8),  // "gpsValid"
        QT_MOC_LITERAL(1214, 13),  // "gpsSourceText"
        QT_MOC_LITERAL(1228, 11),  // "gpsLatitude"
        QT_MOC_LITERAL(1240, 12),  // "gpsLongitude"
        QT_MOC_LITERAL(1253, 11),  // "gpsSpeedKph"
        QT_MOC_LITERAL(1265, 9),  // "gpsCourse"
        QT_MOC_LITERAL(1275, 8),  // "tractorX"
        QT_MOC_LITERAL(1284, 8),  // "tractorZ"
        QT_MOC_LITERAL(1293, 11),  // "trackPoints"
        QT_MOC_LITERAL(1305, 12),  // "workedPoints"
        QT_MOC_LITERAL(1318, 19),  // "fieldBoundaryPoints"
        QT_MOC_LITERAL(1338, 17),  // "boundaryRecording"
        QT_MOC_LITERAL(1356, 18),  // "boundaryPointCount"
        QT_MOC_LITERAL(1375, 10),  // "fieldNames"
        QT_MOC_LITERAL(1386, 9),  // "taskNames"
        QT_MOC_LITERAL(1396, 18),  // "selectedFieldIndex"
        QT_MOC_LITERAL(1415, 17),  // "selectedTaskIndex"
        QT_MOC_LITERAL(1433, 15),  // "activeFieldName"
        QT_MOC_LITERAL(1449, 14),  // "activeTaskName"
        QT_MOC_LITERAL(1464, 11),  // "fieldWidthM"
        QT_MOC_LITERAL(1476, 12),  // "fieldLengthM"
        QT_MOC_LITERAL(1489, 13),  // "implementName"
        QT_MOC_LITERAL(1503, 23),  // "implementGeometryStatus"
        QT_MOC_LITERAL(1527, 17),  // "implementElements"
        QT_MOC_LITERAL(1545, 13),  // "implementDdis"
        QT_MOC_LITERAL(1559, 11),  // "autoDdiSync"
        QT_MOC_LITERAL(1571, 17),  // "ddiSyncIntervalMs"
        QT_MOC_LITERAL(1589, 11),  // "tcBasicData"
        QT_MOC_LITERAL(1601, 18),  // "activeSectionCount"
        QT_MOC_LITERAL(1620, 12),  // "workedAreaHa"
        QT_MOC_LITERAL(1633, 15),  // "workedDistanceM"
        QT_MOC_LITERAL(1649, 17),  // "workedTimeSeconds"
        QT_MOC_LITERAL(1667, 13),  // "steeringAngle"
        QT_MOC_LITERAL(1681, 11),  // "throttleKph"
        QT_MOC_LITERAL(1693, 10),  // "implementX"
        QT_MOC_LITERAL(1704, 10),  // "implementZ"
        QT_MOC_LITERAL(1715, 15),  // "implementCourse"
        QT_MOC_LITERAL(1731, 11),  // "clientModel"
        QT_MOC_LITERAL(1743, 16),  // "ClientListModel*"
        QT_MOC_LITERAL(1760, 9),  // "ddopModel"
        QT_MOC_LITERAL(1770, 10),  // "DdopModel*"
        QT_MOC_LITERAL(1781, 10),  // "valueModel"
        QT_MOC_LITERAL(1792, 17),  // "ProcessDataModel*"
        QT_MOC_LITERAL(1810, 8),  // "logModel"
        QT_MOC_LITERAL(1819, 9),  // "LogModel*"
        QT_MOC_LITERAL(1829, 15),  // "MeasurementKind"
        QT_MOC_LITERAL(1845, 12),  // "TimeInterval"
        QT_MOC_LITERAL(1858, 16),  // "DistanceInterval"
        QT_MOC_LITERAL(1875, 16),  // "MinimumThreshold"
        QT_MOC_LITERAL(1892, 16),  // "MaximumThreshold"
        QT_MOC_LITERAL(1909, 15)   // "ChangeThreshold"
    },
    "agisotc::TcBridge",
    "runningChanged",
    "",
    "taskActiveChanged",
    "selectedClientChanged",
    "sectionDdiChanged",
    "sectionCountChanged",
    "sectionStatesChanged",
    "statusTextChanged",
    "gpsChanged",
    "trackChanged",
    "fieldsChanged",
    "tasksChanged",
    "implementChanged",
    "implementDdisChanged",
    "autoDdiSyncChanged",
    "boundaryChanged",
    "workChanged",
    "drivingControlsChanged",
    "identifyBanner",
    "tcNumber",
    "startServer",
    "driver",
    "channel",
    "booms",
    "sections",
    "channels",
    "stopServer",
    "poll",
    "selectClient",
    "address",
    "setTaskActive",
    "active",
    "requestValue",
    "ddi",
    "element",
    "setValue",
    "value",
    "acknowledge",
    "sendMeasurement",
    "kind",
    "setSectionDdi",
    "setSectionCount",
    "count",
    "loadPoolFile",
    "fileUrl",
    "clearPool",
    "clearLog",
    "startGps",
    "source",
    "serialPort",
    "baudRate",
    "latitude",
    "longitude",
    "stopGps",
    "setSimulationMotion",
    "speedKph",
    "courseDeg",
    "nudgeSimulation",
    "forwardMeters",
    "turnDegrees",
    "createField",
    "name",
    "widthM",
    "lengthM",
    "selectField",
    "index",
    "createTask",
    "selectTask",
    "startSelectedTask",
    "pauseSelectedTask",
    "stopSelectedTask",
    "clearTrack",
    "setAutoDdiSync",
    "enabled",
    "setDdiSyncIntervalMs",
    "intervalMs",
    "requestImplementDdis",
    "startBoundaryRecording",
    "finishBoundaryRecording",
    "cancelBoundaryRecording",
    "setSteeringAngle",
    "degrees",
    "setThrottleKph",
    "adjustThrottle",
    "deltaKph",
    "stopTractor",
    "clearWorkedArea",
    "running",
    "taskActive",
    "selectedClient",
    "sectionDdi",
    "sectionCount",
    "sectionStates",
    "statusText",
    "gpsRunning",
    "gpsValid",
    "gpsSourceText",
    "gpsLatitude",
    "gpsLongitude",
    "gpsSpeedKph",
    "gpsCourse",
    "tractorX",
    "tractorZ",
    "trackPoints",
    "workedPoints",
    "fieldBoundaryPoints",
    "boundaryRecording",
    "boundaryPointCount",
    "fieldNames",
    "taskNames",
    "selectedFieldIndex",
    "selectedTaskIndex",
    "activeFieldName",
    "activeTaskName",
    "fieldWidthM",
    "fieldLengthM",
    "implementName",
    "implementGeometryStatus",
    "implementElements",
    "implementDdis",
    "autoDdiSync",
    "ddiSyncIntervalMs",
    "tcBasicData",
    "activeSectionCount",
    "workedAreaHa",
    "workedDistanceM",
    "workedTimeSeconds",
    "steeringAngle",
    "throttleKph",
    "implementX",
    "implementZ",
    "implementCourse",
    "clientModel",
    "ClientListModel*",
    "ddopModel",
    "DdopModel*",
    "valueModel",
    "ProcessDataModel*",
    "logModel",
    "LogModel*",
    "MeasurementKind",
    "TimeInterval",
    "DistanceInterval",
    "MinimumThreshold",
    "MaximumThreshold",
    "ChangeThreshold"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSagisotcSCOPETcBridgeENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      54,   14, // methods
      49,  478, // properties
       1,  723, // enums/sets
       0,    0, // constructors
       0,       // flags
      18,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  338,    2, 0x06,   50 /* Public */,
       3,    0,  339,    2, 0x06,   51 /* Public */,
       4,    0,  340,    2, 0x06,   52 /* Public */,
       5,    0,  341,    2, 0x06,   53 /* Public */,
       6,    0,  342,    2, 0x06,   54 /* Public */,
       7,    0,  343,    2, 0x06,   55 /* Public */,
       8,    0,  344,    2, 0x06,   56 /* Public */,
       9,    0,  345,    2, 0x06,   57 /* Public */,
      10,    0,  346,    2, 0x06,   58 /* Public */,
      11,    0,  347,    2, 0x06,   59 /* Public */,
      12,    0,  348,    2, 0x06,   60 /* Public */,
      13,    0,  349,    2, 0x06,   61 /* Public */,
      14,    0,  350,    2, 0x06,   62 /* Public */,
      15,    0,  351,    2, 0x06,   63 /* Public */,
      16,    0,  352,    2, 0x06,   64 /* Public */,
      17,    0,  353,    2, 0x06,   65 /* Public */,
      18,    0,  354,    2, 0x06,   66 /* Public */,
      19,    1,  355,    2, 0x06,   67 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      21,    6,  358,    2, 0x02,   69 /* Public */,
      27,    0,  371,    2, 0x02,   76 /* Public */,
      28,    0,  372,    2, 0x02,   77 /* Public */,
      29,    1,  373,    2, 0x02,   78 /* Public */,
      31,    1,  376,    2, 0x02,   80 /* Public */,
      33,    2,  379,    2, 0x02,   82 /* Public */,
      36,    4,  384,    2, 0x02,   85 /* Public */,
      39,    4,  393,    2, 0x02,   90 /* Public */,
      41,    1,  402,    2, 0x02,   95 /* Public */,
      42,    1,  405,    2, 0x02,   97 /* Public */,
      44,    1,  408,    2, 0x02,   99 /* Public */,
      46,    0,  411,    2, 0x02,  101 /* Public */,
      47,    0,  412,    2, 0x02,  102 /* Public */,
      48,    5,  413,    2, 0x02,  103 /* Public */,
      54,    0,  424,    2, 0x02,  109 /* Public */,
      55,    2,  425,    2, 0x02,  110 /* Public */,
      58,    2,  430,    2, 0x02,  113 /* Public */,
      61,    3,  435,    2, 0x02,  116 /* Public */,
      65,    1,  442,    2, 0x02,  120 /* Public */,
      67,    1,  445,    2, 0x02,  122 /* Public */,
      68,    1,  448,    2, 0x02,  124 /* Public */,
      69,    0,  451,    2, 0x02,  126 /* Public */,
      70,    0,  452,    2, 0x02,  127 /* Public */,
      71,    0,  453,    2, 0x02,  128 /* Public */,
      72,    0,  454,    2, 0x02,  129 /* Public */,
      73,    1,  455,    2, 0x02,  130 /* Public */,
      75,    1,  458,    2, 0x02,  132 /* Public */,
      77,    0,  461,    2, 0x02,  134 /* Public */,
      78,    1,  462,    2, 0x02,  135 /* Public */,
      79,    0,  465,    2, 0x02,  137 /* Public */,
      80,    0,  466,    2, 0x02,  138 /* Public */,
      81,    1,  467,    2, 0x02,  139 /* Public */,
      83,    1,  470,    2, 0x02,  141 /* Public */,
      84,    1,  473,    2, 0x02,  143 /* Public */,
      86,    0,  476,    2, 0x02,  145 /* Public */,
      87,    0,  477,    2, 0x02,  146 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   20,

 // methods: parameters
    QMetaType::Bool, QMetaType::QString, QMetaType::QString, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   22,   23,   20,   24,   25,   26,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   30,
    QMetaType::Void, QMetaType::Bool,   32,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   34,   35,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Bool,   34,   35,   37,   38,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   40,   34,   35,   37,
    QMetaType::Void, QMetaType::Int,   34,
    QMetaType::Void, QMetaType::Int,   43,
    QMetaType::Void, QMetaType::QUrl,   45,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString, QMetaType::Int, QMetaType::Double, QMetaType::Double,   49,   50,   51,   52,   53,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   56,   57,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   59,   60,
    QMetaType::Bool, QMetaType::QString, QMetaType::Double, QMetaType::Double,   62,   63,   64,
    QMetaType::Void, QMetaType::Int,   66,
    QMetaType::Bool, QMetaType::QString,   62,
    QMetaType::Void, QMetaType::Int,   66,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   74,
    QMetaType::Void, QMetaType::Int,   76,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString,   62,
    QMetaType::Bool,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double,   82,
    QMetaType::Void, QMetaType::Double,   56,
    QMetaType::Void, QMetaType::Double,   85,
    QMetaType::Void,
    QMetaType::Void,

 // properties: name, type, flags
      88, QMetaType::Bool, 0x00015001, uint(0), 0,
      89, QMetaType::Bool, 0x00015001, uint(1), 0,
      90, QMetaType::Int, 0x00015001, uint(2), 0,
      91, QMetaType::Int, 0x00015001, uint(3), 0,
      92, QMetaType::Int, 0x00015001, uint(4), 0,
      93, QMetaType::QVariantList, 0x00015001, uint(5), 0,
      94, QMetaType::QString, 0x00015001, uint(6), 0,
      95, QMetaType::Bool, 0x00015001, uint(7), 0,
      96, QMetaType::Bool, 0x00015001, uint(7), 0,
      97, QMetaType::QString, 0x00015001, uint(7), 0,
      98, QMetaType::Double, 0x00015001, uint(7), 0,
      99, QMetaType::Double, 0x00015001, uint(7), 0,
     100, QMetaType::Double, 0x00015001, uint(7), 0,
     101, QMetaType::Double, 0x00015001, uint(7), 0,
     102, QMetaType::Double, 0x00015001, uint(7), 0,
     103, QMetaType::Double, 0x00015001, uint(7), 0,
     104, QMetaType::QVariantList, 0x00015001, uint(8), 0,
     105, QMetaType::QVariantList, 0x00015001, uint(15), 0,
     106, QMetaType::QVariantList, 0x00015001, uint(14), 0,
     107, QMetaType::Bool, 0x00015001, uint(14), 0,
     108, QMetaType::Int, 0x00015001, uint(14), 0,
     109, QMetaType::QStringList, 0x00015001, uint(9), 0,
     110, QMetaType::QStringList, 0x00015001, uint(10), 0,
     111, QMetaType::Int, 0x00015001, uint(9), 0,
     112, QMetaType::Int, 0x00015001, uint(10), 0,
     113, QMetaType::QString, 0x00015001, uint(9), 0,
     114, QMetaType::QString, 0x00015001, uint(10), 0,
     115, QMetaType::Double, 0x00015001, uint(9), 0,
     116, QMetaType::Double, 0x00015001, uint(9), 0,
     117, QMetaType::QString, 0x00015001, uint(11), 0,
     118, QMetaType::QString, 0x00015001, uint(11), 0,
     119, QMetaType::QVariantList, 0x00015001, uint(11), 0,
     120, QMetaType::QVariantList, 0x00015001, uint(12), 0,
     121, QMetaType::Bool, 0x00015001, uint(13), 0,
     122, QMetaType::Int, 0x00015001, uint(13), 0,
     123, QMetaType::QVariantList, 0x00015001, uint(12), 0,
     124, QMetaType::Int, 0x00015001, uint(5), 0,
     125, QMetaType::Double, 0x00015001, uint(15), 0,
     126, QMetaType::Double, 0x00015001, uint(15), 0,
     127, QMetaType::Double, 0x00015001, uint(15), 0,
     128, QMetaType::Double, 0x00015001, uint(16), 0,
     129, QMetaType::Double, 0x00015001, uint(16), 0,
     130, QMetaType::Double, 0x00015001, uint(7), 0,
     131, QMetaType::Double, 0x00015001, uint(7), 0,
     132, QMetaType::Double, 0x00015001, uint(7), 0,
     133, 0x80000000 | 134, 0x00015409, uint(-1), 0,
     135, 0x80000000 | 136, 0x00015409, uint(-1), 0,
     137, 0x80000000 | 138, 0x00015409, uint(-1), 0,
     139, 0x80000000 | 140, 0x00015409, uint(-1), 0,

 // enums: name, alias, flags, count, data
     141,  141, 0x2,    5,  728,

 // enum data: key, value
     142, uint(agisotc::TcBridge::MeasurementKind::TimeInterval),
     143, uint(agisotc::TcBridge::MeasurementKind::DistanceInterval),
     144, uint(agisotc::TcBridge::MeasurementKind::MinimumThreshold),
     145, uint(agisotc::TcBridge::MeasurementKind::MaximumThreshold),
     146, uint(agisotc::TcBridge::MeasurementKind::ChangeThreshold),

       0        // eod
};

Q_CONSTINIT const QMetaObject agisotc::TcBridge::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSagisotcSCOPETcBridgeENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS_t,
        // property 'running'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'taskActive'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'selectedClient'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'sectionDdi'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'sectionCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'sectionStates'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'statusText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'gpsRunning'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'gpsValid'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'gpsSourceText'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'gpsLatitude'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'gpsLongitude'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'gpsSpeedKph'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'gpsCourse'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'tractorX'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'tractorZ'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'trackPoints'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'workedPoints'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'fieldBoundaryPoints'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'boundaryRecording'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'boundaryPointCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'fieldNames'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'taskNames'
        QtPrivate::TypeAndForceComplete<QStringList, std::true_type>,
        // property 'selectedFieldIndex'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'selectedTaskIndex'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'activeFieldName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'activeTaskName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'fieldWidthM'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'fieldLengthM'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'implementName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'implementGeometryStatus'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'implementElements'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'implementDdis'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'autoDdiSync'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'ddiSyncIntervalMs'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'tcBasicData'
        QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>,
        // property 'activeSectionCount'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'workedAreaHa'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'workedDistanceM'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'workedTimeSeconds'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'steeringAngle'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'throttleKph'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'implementX'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'implementZ'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'implementCourse'
        QtPrivate::TypeAndForceComplete<double, std::true_type>,
        // property 'clientModel'
        QtPrivate::TypeAndForceComplete<ClientListModel*, std::true_type>,
        // property 'ddopModel'
        QtPrivate::TypeAndForceComplete<DdopModel*, std::true_type>,
        // property 'valueModel'
        QtPrivate::TypeAndForceComplete<ProcessDataModel*, std::true_type>,
        // property 'logModel'
        QtPrivate::TypeAndForceComplete<LogModel*, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TcBridge, std::true_type>,
        // method 'runningChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'taskActiveChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectedClientChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sectionDdiChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sectionCountChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sectionStatesChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'gpsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'trackChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'fieldsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'tasksChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'implementChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'implementDdisChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'autoDdiSyncChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'boundaryChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'workChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'drivingControlsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'identifyBanner'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'startServer'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'stopServer'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'poll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectClient'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setTaskActive'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'requestValue'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setValue'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'sendMeasurement'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setSectionDdi'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setSectionCount'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'loadPoolFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'clearPool'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearLog'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startGps'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'stopGps'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setSimulationMotion'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'nudgeSimulation'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'createField'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'selectField'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'createTask'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'selectTask'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'startSelectedTask'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'pauseSelectedTask'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stopSelectedTask'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearTrack'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setAutoDdiSync'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'setDdiSyncIntervalMs'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'requestImplementDdis'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startBoundaryRecording'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'finishBoundaryRecording'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'cancelBoundaryRecording'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setSteeringAngle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'setThrottleKph'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'adjustThrottle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'stopTractor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clearWorkedArea'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void agisotc::TcBridge::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TcBridge *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->runningChanged(); break;
        case 1: _t->taskActiveChanged(); break;
        case 2: _t->selectedClientChanged(); break;
        case 3: _t->sectionDdiChanged(); break;
        case 4: _t->sectionCountChanged(); break;
        case 5: _t->sectionStatesChanged(); break;
        case 6: _t->statusTextChanged(); break;
        case 7: _t->gpsChanged(); break;
        case 8: _t->trackChanged(); break;
        case 9: _t->fieldsChanged(); break;
        case 10: _t->tasksChanged(); break;
        case 11: _t->implementChanged(); break;
        case 12: _t->implementDdisChanged(); break;
        case 13: _t->autoDdiSyncChanged(); break;
        case 14: _t->boundaryChanged(); break;
        case 15: _t->workChanged(); break;
        case 16: _t->drivingControlsChanged(); break;
        case 17: _t->identifyBanner((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 18: { bool _r = _t->startServer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[5])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[6])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 19: _t->stopServer(); break;
        case 20: _t->poll(); break;
        case 21: _t->selectClient((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 22: _t->setTaskActive((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 23: _t->requestValue((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 24: _t->setValue((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[4]))); break;
        case 25: _t->sendMeasurement((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        case 26: _t->setSectionDdi((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 27: _t->setSectionCount((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 28: _t->loadPoolFile((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1]))); break;
        case 29: _t->clearPool(); break;
        case 30: _t->clearLog(); break;
        case 31: { bool _r = _t->startGps((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[5])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 32: _t->stopGps(); break;
        case 33: _t->setSimulationMotion((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2]))); break;
        case 34: _t->nudgeSimulation((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2]))); break;
        case 35: { bool _r = _t->createField((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 36: _t->selectField((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 37: { bool _r = _t->createTask((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 38: _t->selectTask((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 39: _t->startSelectedTask(); break;
        case 40: _t->pauseSelectedTask(); break;
        case 41: _t->stopSelectedTask(); break;
        case 42: _t->clearTrack(); break;
        case 43: _t->setAutoDdiSync((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 44: _t->setDdiSyncIntervalMs((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 45: _t->requestImplementDdis(); break;
        case 46: { bool _r = _t->startBoundaryRecording((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 47: { bool _r = _t->finishBoundaryRecording();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 48: _t->cancelBoundaryRecording(); break;
        case 49: _t->setSteeringAngle((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 50: _t->setThrottleKph((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 51: _t->adjustThrottle((*reinterpret_cast< std::add_pointer_t<double>>(_a[1]))); break;
        case 52: _t->stopTractor(); break;
        case 53: _t->clearWorkedArea(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::runningChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::taskActiveChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::selectedClientChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::sectionDdiChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::sectionCountChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::sectionStatesChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::statusTextChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::gpsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::trackChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::fieldsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::tasksChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::implementChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::implementDdisChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::autoDdiSyncChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::boundaryChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::workChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)();
            if (_t _q_method = &TcBridge::drivingControlsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (TcBridge::*)(int );
            if (_t _q_method = &TcBridge::identifyBanner; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
    } else if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 45:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ClientListModel* >(); break;
        case 46:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< DdopModel* >(); break;
        case 48:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< LogModel* >(); break;
        case 47:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ProcessDataModel* >(); break;
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<TcBridge *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isRunning(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->isTaskActive(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->selectedClient(); break;
        case 3: *reinterpret_cast< int*>(_v) = _t->sectionDdi(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->sectionCount(); break;
        case 5: *reinterpret_cast< QVariantList*>(_v) = _t->sectionStates(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->statusText(); break;
        case 7: *reinterpret_cast< bool*>(_v) = _t->isGpsRunning(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->isGpsValid(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->gpsSourceText(); break;
        case 10: *reinterpret_cast< double*>(_v) = _t->gpsLatitude(); break;
        case 11: *reinterpret_cast< double*>(_v) = _t->gpsLongitude(); break;
        case 12: *reinterpret_cast< double*>(_v) = _t->gpsSpeedKph(); break;
        case 13: *reinterpret_cast< double*>(_v) = _t->gpsCourse(); break;
        case 14: *reinterpret_cast< double*>(_v) = _t->tractorX(); break;
        case 15: *reinterpret_cast< double*>(_v) = _t->tractorZ(); break;
        case 16: *reinterpret_cast< QVariantList*>(_v) = _t->trackPoints(); break;
        case 17: *reinterpret_cast< QVariantList*>(_v) = _t->workedPoints(); break;
        case 18: *reinterpret_cast< QVariantList*>(_v) = _t->fieldBoundaryPoints(); break;
        case 19: *reinterpret_cast< bool*>(_v) = _t->boundaryRecording(); break;
        case 20: *reinterpret_cast< int*>(_v) = _t->boundaryPointCount(); break;
        case 21: *reinterpret_cast< QStringList*>(_v) = _t->fieldNames(); break;
        case 22: *reinterpret_cast< QStringList*>(_v) = _t->taskNames(); break;
        case 23: *reinterpret_cast< int*>(_v) = _t->selectedFieldIndex(); break;
        case 24: *reinterpret_cast< int*>(_v) = _t->selectedTaskIndex(); break;
        case 25: *reinterpret_cast< QString*>(_v) = _t->activeFieldName(); break;
        case 26: *reinterpret_cast< QString*>(_v) = _t->activeTaskName(); break;
        case 27: *reinterpret_cast< double*>(_v) = _t->fieldWidthM(); break;
        case 28: *reinterpret_cast< double*>(_v) = _t->fieldLengthM(); break;
        case 29: *reinterpret_cast< QString*>(_v) = _t->implementName(); break;
        case 30: *reinterpret_cast< QString*>(_v) = _t->implementGeometryStatus(); break;
        case 31: *reinterpret_cast< QVariantList*>(_v) = _t->implementElements(); break;
        case 32: *reinterpret_cast< QVariantList*>(_v) = _t->implementDdis(); break;
        case 33: *reinterpret_cast< bool*>(_v) = _t->autoDdiSync(); break;
        case 34: *reinterpret_cast< int*>(_v) = _t->ddiSyncIntervalMs(); break;
        case 35: *reinterpret_cast< QVariantList*>(_v) = _t->tcBasicData(); break;
        case 36: *reinterpret_cast< int*>(_v) = _t->activeSectionCount(); break;
        case 37: *reinterpret_cast< double*>(_v) = _t->workedAreaHa(); break;
        case 38: *reinterpret_cast< double*>(_v) = _t->workedDistanceM(); break;
        case 39: *reinterpret_cast< double*>(_v) = _t->workedTimeSeconds(); break;
        case 40: *reinterpret_cast< double*>(_v) = _t->steeringAngle(); break;
        case 41: *reinterpret_cast< double*>(_v) = _t->throttleKph(); break;
        case 42: *reinterpret_cast< double*>(_v) = _t->implementX(); break;
        case 43: *reinterpret_cast< double*>(_v) = _t->implementZ(); break;
        case 44: *reinterpret_cast< double*>(_v) = _t->implementCourse(); break;
        case 45: *reinterpret_cast< ClientListModel**>(_v) = _t->clientModel(); break;
        case 46: *reinterpret_cast< DdopModel**>(_v) = _t->ddopModel(); break;
        case 47: *reinterpret_cast< ProcessDataModel**>(_v) = _t->valueModel(); break;
        case 48: *reinterpret_cast< LogModel**>(_v) = _t->logModel(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *agisotc::TcBridge::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *agisotc::TcBridge::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSagisotcSCOPETcBridgeENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int agisotc::TcBridge::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 54)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 54;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 54)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 54;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 49;
    }
    return _id;
}

// SIGNAL 0
void agisotc::TcBridge::runningChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void agisotc::TcBridge::taskActiveChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void agisotc::TcBridge::selectedClientChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void agisotc::TcBridge::sectionDdiChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void agisotc::TcBridge::sectionCountChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void agisotc::TcBridge::sectionStatesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void agisotc::TcBridge::statusTextChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void agisotc::TcBridge::gpsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void agisotc::TcBridge::trackChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void agisotc::TcBridge::fieldsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void agisotc::TcBridge::tasksChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void agisotc::TcBridge::implementChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void agisotc::TcBridge::implementDdisChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void agisotc::TcBridge::autoDdiSyncChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void agisotc::TcBridge::boundaryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void agisotc::TcBridge::workChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void agisotc::TcBridge::drivingControlsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 16, nullptr);
}

// SIGNAL 17
void agisotc::TcBridge::identifyBanner(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}
QT_WARNING_POP
