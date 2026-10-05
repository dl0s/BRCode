import bb.cascades 1.4
import bb.system 1.2
import QtQuick 1.0 as Quick

Page {
    id: root
    actionBarVisibility: ChromeVisibility.Hidden
    keysIgnoreFocusInActionBar: true
    Container {
        background: Color.create("#15191d")
        leftPadding: 12
        rightPadding: 12
        topPadding: 4
        bottomPadding: 6
        Container {
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
            minHeight: 64
            maxHeight: 64
            Label {
                text: app.page === 3 ? "记录详情" : "BRCode"
                textStyle.fontSize: FontSize.Large
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            }
        }
        SegmentedControl {
            id: navigation
            Option { text: "扫描"; selected: app.page === 0 }
            Option { text: "生成"; selected: app.page === 1 }
            Option { text: "历史"; selected: app.page === 2 || app.page === 3 }
            onSelectedIndexChanged: {
                if (selectedIndex >= 0 && !(selectedIndex === 2 && app.page === 3)) app.setPage(selectedIndex);
            }
        }
        Label {
            visible: app.message.length > 0
            text: app.message
            textFormat: TextFormat.Plain
            multiline: true
            textStyle.fontSize: FontSize.XSmall
            textStyle.color: Color.create("#83dfb5")
            maxHeight: 100
        }
        Button { visible: app.pendingSave; text: "重试保存当前结果"; onClicked: app.retrySave() }
        Container {
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            horizontalAlignment: HorizontalAlignment.Fill
            Container {
                visible: app.page === 0
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                Label { text: scanner.status; textFormat: TextFormat.Plain; multiline: true; textStyle.fontSize: FontSize.Small }
                Container {
                    objectName: "cameraPreview"
                    background: Color.Black
                    layout: DockLayout {}
                    horizontalAlignment: HorizontalAlignment.Fill
                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                }
                Button { text: "重试 / 继续扫描"; onClicked: app.continueScan() }
            }
            ScrollView {
                visible: app.page === 1
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                horizontalAlignment: HorizontalAlignment.Fill
                scrollViewProperties.scrollMode: ScrollMode.Vertical
                Container {
                    SystemTextArea {
                        id: input
                        objectName: "generatorText"
                        scopeActive: app.page === 1
                        onCreationCompleted: text = app.draft
                        hintText: "输入或粘贴原文，保留空格和换行"
                        preferredHeight: 190
                        minHeight: 160
                        onTextChanged: app.draft = text
                        onTextChanging: app.draft = text
                    }
                    Container {
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        Button { text: "生成"; onClicked: app.generate(input) }
                        Button { text: "粘贴"; onClicked: app.paste(input) }
                        Button { text: "复制原文"; onClicked: app.copyText(input.text) }
                    }
                    Container {
                        visible: app.hasQr
                        horizontalAlignment: HorizontalAlignment.Center
                        background: Color.White
                        ImageView {
                            image: app.qrImage
                            scalingMethod: ScalingMethod.None
                            preferredWidth: app.qrPixels
                            preferredHeight: app.qrPixels
                            minWidth: app.qrPixels
                            minHeight: app.qrPixels
                            maxWidth: app.qrPixels
                            maxHeight: app.qrPixels
                            loadEffect: ImageViewLoadEffect.None
                        }
                    }
                    Container {
                        visible: app.hasQr
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        Button { text: "全屏显示"; onClicked: { app.suspendInput(); qrScreen.open(); } }
                        Button { text: "保存 PNG"; onClicked: app.exportPng() }
                    }
                    Button { visible: !app.saved && app.hasQr; text: "重试保存历史"; onClicked: app.retrySave() }
                    Label { text: "离线生成 · 黑码白底 · 原文不裁剪\n使用系统输入法，Enter 换行"; multiline: true; textStyle.fontSize: FontSize.XSmall }
                }
            }
            Container {
                visible: app.page === 2
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                SystemTextField {
                    id: search
                    objectName: "historySearch"
                    scopeActive: app.page === 2
                    hintText: "搜索正文、标题或备注"
                    onTextChanged: app.search(text)
                }
                ListView {
                    objectName: "historyList"
                    dataModel: app.historyModel
                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    listItemComponents: [
                        ListItemComponent {
                            type: ""
                            StandardListItem { title: ListItemData.title; description: ListItemData.summary; status: ListItemData.created.substr(0, 10) }
                        }
                    ]
                    onTriggered: app.viewRecord(dataModel.data(indexPath).id)
                }
                Button { visible: app.hasMore; text: "加载更多"; onClicked: app.loadMore() }
            }
            ScrollView {
                visible: app.page === 3
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                horizontalAlignment: HorizontalAlignment.Fill
                scrollViewProperties.scrollMode: ScrollMode.Vertical
                Container {
                    Label { text: (app.detail.source === "scan" ? "扫描" : "生成") + " · " + (app.saved ? "已保存" : "保存失败，可重试") + "\n" + app.detail.created; textFormat: TextFormat.Plain; multiline: true; textStyle.fontSize: FontSize.XSmall }
                    TextArea {
                        objectName: "resultText"
                        text: app.detail.body
                        textFormat: TextFormat.Plain
                        editable: false
                        preferredHeight: 260
                    }
                    Container {
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        Button { text: "复制"; onClicked: app.copyText(app.detail.body) }
                        Button { text: "编辑副本"; onClicked: app.regenerate() }
                        Button { visible: app.canOpenUrl; text: "打开链接"; onClicked: app.openUrl() }
                    }
                    SystemTextField { id: title; objectName: "recordTitle"; scopeActive: app.page === 3; hintText: "标题（可选）"; text: app.detail.title }
                    SystemTextArea { id: notes; objectName: "recordNotes"; scopeActive: app.page === 3; hintText: "备注（可选）"; text: app.detail.notes; preferredHeight: 140 }
                    Container {
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        Button {
                            text: "保存备注"
                            onClicked: app.saveNotes(title, notes)
                        }
                        Button { visible: !app.saved; text: "重试保存"; onClicked: app.retrySave() }
                        Button { visible: app.saved; text: "删除"; onClicked: { app.suspendInput(); deletion.show(); } }
                    }
                    Container {
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        Button { text: "继续扫描"; onClicked: app.continueScan() }
                        Button { text: "返回历史"; onClicked: app.setPage(2) }
                    }
                }
            }
        }
    }
    attachedObjects: [
        SystemDialog {
            id: deletion
            title: "删除此记录？"
            body: "此操作删除正文、标题与备注。"
            confirmButton.label: "删除"
            cancelButton.label: "取消"
            onFinished: { if (result === SystemUiResult.ConfirmButtonSelection) app.deleteRecord(); }
        },
        Dialog {
            id: qrScreen
            Container {
                background: Color.White
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                Container {
                    layout: DockLayout {}
                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    horizontalAlignment: HorizontalAlignment.Fill
                    ImageView {
                    image: app.fullImage
                    scalingMethod: ScalingMethod.None
                    preferredWidth: app.fullPixels
                    preferredHeight: app.fullPixels
                    minWidth: app.fullPixels
                    minHeight: app.fullPixels
                    maxWidth: app.fullPixels
                    maxHeight: app.fullPixels
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    loadEffect: ImageViewLoadEffect.None
                    }
                }
                Button { text: "关闭"; horizontalAlignment: HorizontalAlignment.Center; onClicked: qrScreen.close() }
            }
        },
        Quick.Connections {
            target: app
            onDraftReplaced: input.text = text
            onChanged: {
                if (app.page !== 1 || !app.hasQr) qrScreen.close();
            }
        }
    ]
}
