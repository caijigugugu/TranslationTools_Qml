import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import MyTranslate 1.0

Window {
    id: window
    visible: true
    width: 640
    height: 460
    title: qsTr("Translation_Tools")

    property var currentTarget: null
    property bool bIsSelectFolder: false;
    property var myNameFilters: [
        "All files (*)",
        "TS files (*.ts)",
        "Excel files (*.xls *.xlsx)"
    ]
    property string sTsFileToRead: ""
    property string sXmlFileToRead: ""
    property string sTsFileToWrite: ""
    property string sQmPathToConvert: ""
    property var langModel: ListModel {}

    FileDialog {
        id: fileDlg
        nameFilters: myNameFilters
        selectExisting: true  // 必须选择已存在的文件
        selectMultiple: false
        selectFolder: bIsSelectFolder

        onAccepted: {
            var filePath = fileUrl.toString().replace(/^file:\/\/\//, "");
            if(currentTarget)
            {
                if(currentTarget === btnTs_one)
                {
                    //选择生成xml的ts文件
                    sTsFileToRead = filePath

                } else if(currentTarget === btnXml_two)
                {
                    //选择已翻译的xml文件
                    sXmlFileToRead = filePath
                    var langList = MyTranslate.getLanguageOptions(filePath)
                    window.langModel.clear()
                    for (var i = 0; i < langList.length; i++) {
                        window.langModel.append({ id: langList[i].id, text: langList[i]});
                    }
                    if(langList.length > 0)
                    {
                        comboBox.currentIndex = 0;
                    }
                } else if(currentTarget === btnTs_two)
                {
                    //选择将写入翻译的ts文件
                    sTsFileToWrite = filePath
                } else if(currentTarget === btnPath_three)
                {
                    //选择生成qm文件的目录，该目录下应有源ts文件
                    sQmPathToConvert = filePath
                }
            }
        }
    }

    Rectangle {
        id: control
        anchors.margins: 5
        anchors.fill: parent

        Flickable {
            id: flickable
            clip: true
            anchors.fill: parent
            ScrollBar.vertical: ScrollBar {}
            ScrollBar.horizontal: ScrollBar {}
            boundsBehavior: Flickable.StopAtBounds

            contentWidth: Math.max(leftArea.x + leftArea.width, groupBox1.x + groupBox1.width)  // 取最右边的 GroupBox 的右边界
                contentHeight: Math.max(leftArea.y + leftArea.height, groupBox3.y + groupBox3.height)

            GroupBox {
                id: leftArea
                x: 0
                y: 8
                width: 300
                height: 442
                transformOrigin: Item.Center
                title: qsTr("")

                Label {
                    id: statisticalInfo
                    x: -7
                    y: 377
                    width: 290
                    height: 50
                    text: qsTr("统计信息")
                    wrapMode: Text.WordWrap
                    verticalAlignment: Text.AlignVCenter
                }

                ScrollView {
                    x: -6
                    y: 0
                    width: 290
                    height: 374
                    clip: true
                    bottomPadding: 6
                    TextEdit {
                        id: textField
                        x: 0
                        y: 0
                        width: 290
                        selectByMouse: true
                        clip: true
                        wrapMode: Text.WordWrap
                        verticalAlignment: Text.AlignTop

                    }
                }
            }

            GroupBox {
                id: groupBox1
                x: 303
                y: 8
                width: 325
                height: 116
                font.pointSize: 10
                padding: 12
                topPadding: 12
                enabled: true
                title: qsTr("1.ts -> xml")

                Button {
                    id: btnTs_one
                    x: 227
                    y: 15
                    width: 70
                    height: 30
                    text: qsTr("选择ts目录")
                    onClicked: {
                        currentTarget = btnTs_one
                        bIsSelectFolder = true
                        fileDlg.nameFilters = myNameFilters
                        fileDlg.open()
                        bIsSelectFolder = false
                    }
                }

                Button {
                    id: btnXml_one
                    x: 227
                    y: 64
                    width: 70
                    height: 30
                    text: qsTr("生成xml")
                    onClicked: {
                        Qt.callLater(function() {
                            textField.clear()
                            MyTranslate.executeAsync(MyTranslate.TaskParseTsToXml,sTsFileToRead)
                        })
                    }
                }

                Rectangle {
                    id: rec1
                    x: -7
                    y: 8
                    border.width: 1
                    width: 225
                    height: 42
                    TextInput {
                        id: textTsFileGroup1
                        anchors.fill: parent
                        text: sTsFileToRead
                        wrapMode: Text.WrapAnywhere
                        padding: 2
                        mouseSelectionMode: TextInput.SelectWords
                        horizontalAlignment: Text.AlignLeft
                        clip: true
                        selectByMouse: true
                        cursorVisible: false
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15
                    }
                }
            }

            GroupBox {
                id: groupBox2
                x: 303
                y: 127
                width: 325
                height: 212
                font.pointSize: 10
                contentHeight: 0
                Button {
                    id: btnXml_two
                    x: 231
                    y: 8
                    width: 70
                    height: 30
                    text: qsTr("选择xml")
                    onClicked: {
                        currentTarget = btnXml_two
                        fileDlg.nameFilters = myNameFilters
                        fileDlg.selectedNameFilter = myNameFilters[2]
                        fileDlg.open()
                    }
                }

                Button {
                    id: btnTs_two
                    x: 231
                    y: 68
                    width: 70
                    height: 30
                    text: qsTr("选择目标ts")
                    onClicked: {
                        currentTarget = btnTs_two
                        fileDlg.nameFilters = myNameFilters
                        fileDlg.selectedNameFilter = myNameFilters[1]
                        fileDlg.open()
                    }
                }
                Rectangle
                {
                    id: rec2
                    x: -7
                    y: 2
                    border.width: 1
                    width: 232
                    height: 46
                    TextInput {
                        id: textEdit1
                        anchors.fill: parent
                        text: sXmlFileToRead
                        wrapMode: Text.WrapAnywhere
                        padding: 2
                        clip: true
                        selectByMouse: true
                        cursorVisible: false
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 15
                    }
                }
                Label {
                    id: label1
                    x: 5
                    y: 110
                    width: 60
                    height: 39
                    text: qsTr("语言")
                    font.pointSize: 12
                    verticalAlignment: Text.AlignVCenter
                }

                Button {
                    id: btnWriteTs_two
                    x: 231
                    y: 148
                    width: 70
                    height: 30
                    text: qsTr("写入ts")
                    onClicked: {
                        textField.clear()
                        console.log("111",sXmlFileToRead,sTsFileToWrite,comboBox.currentText)
                        MyTranslate.executeAsync(MyTranslate.TaskParseXmlToTs
                                                 ,sXmlFileToRead
                                                 ,sTsFileToWrite
                                                 ,comboBox.currentText)
                    }
                }

                ComboBox {
                    id: comboBox
                    x: 65
                    y: 115
                    height: 30
                    wheelEnabled: true
                    enabled: true
                    currentIndex: -1
                    model: langModel
                }

                Rectangle {
                    id: rec3
                    x: -7
                    y: 62
                    border.width: 1
                    width: 232
                    height: 43
                    TextInput {
                        id: textEdit3
                        anchors.fill: parent
                        padding: 2
                        clip: true
                        text: sTsFileToWrite
                        wrapMode: Text.WrapAnywhere
                        selectByMouse: true
                        cursorVisible: false
                        horizontalAlignment: Text.AlignLeft
                        font.pixelSize: 15
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                title: qsTr("2.xml -> ts")
            }

            GroupBox {
                id: groupBox3
                x: 303
                y: 350
                width: 325
                height: 100
                font.pointSize: 10
                Button {
                    id: btnPath_three
                    x: 231
                    y: -3
                    width: 70
                    height: 30
                    text: qsTr("目标文件夹")
                    onClicked: {
                        currentTarget = btnPath_three
                        bIsSelectFolder = true
                        fileDlg.nameFilters = myNameFilters
                        fileDlg.open()
                        bIsSelectFolder = false
                    }
                }

                Button {
                    id: btnQm_three
                    x: 231
                    y: 38
                    width: 70
                    height: 30
                    text: qsTr("生成qm")
                    onClicked: {
                        textField.clear()
                        MyTranslate.executeAsync(MyTranslate.TaskExtractTsToQm
                                                 ,sQmPathToConvert)
                    }
                }
                Rectangle {
                    id:rec4
                    x: -7
                    y: 8
                    border.width: 1
                    width: 233
                    height: 43
                    TextInput {
                        id: textEdit2
                        anchors.fill: parent
                        padding: 2
                        text: window.sQmPathToConvert
                        wrapMode: Text.WrapAnywhere
                        mouseSelectionMode: TextInput.SelectCharacters
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignLeft
                        clip: true
                        selectByMouse: true
                        cursorVisible: false
                        font.pixelSize: 15
                    }
                }

                title: qsTr("1.ts -> qm")
            }
        }
    }

    Component.onCompleted: {
        MyTranslate.logReceived.connect(function(message) {
               textField.append(message);
           });
        MyTranslate.statsReceived.connect(function(total, succeed, failed) {
            statisticalInfo.text = qsTr("统计信息：总共：%1，成功：%2，失败：%3").arg(total).arg(succeed).arg(failed);
           });
    }

}
