import kivy
kivy.require('1.8.0')
from kivy.config import Config
Config.set('graphics', 'width', '1024')
Config.set('graphics', 'height', '576')
from kivy.app import App, Builder
from kivy.graphics import Color, Rectangle
from kivy.uix.floatlayout import FloatLayout
from kivy.uix.button import Button
from kivy.uix.spinner import Spinner
from kivy.uix.gridlayout import GridLayout
from kivy.uix.scrollview import ScrollView
from kivy.uix.spinner import Spinner
from kivy.uix.boxlayout import BoxLayout
from kivy.uix.label import Label
from kivy.uix.accordion import Accordion, AccordionItem
from kivy.uix.treeview import TreeView, TreeViewLabel
from kivy.extras.highlight import KivyLexer
from kivy.uix.codeinput import CodeInput
from pygments import lexers

from spacer import *
from boundedlabel import BoundedLabel
from rcpserial import *
from analogchannelsview import *
from rcpconfig import *
from channels import *

class GPIOChannel(BoxLayout):
    def __init__(self, **kwargs):
        super(GPIOChannel, self).__init__(**kwargs)

class PulseChannel(BoxLayout):
    def __init__(self, **kwargs):
        super(PulseChannel, self).__init__(**kwargs)

class AnalogPulseOutputChannel(BoxLayout):
    def __init__(self, **kwargs):
        super(AnalogPulseOutputChannel, self).__init__(**kwargs)

class OrientationSpinner(Spinner):
    def __init__(self, **kwargs):
        super(OrientationSpinner, self).__init__(**kwargs)
        self.values = ["Normal", "Inverted"]

class ChannelNameSpinner(Spinner):
    def __init__(self, **kwargs):
        self.register_event_type('on_channels_updated')
        super(ChannelNameSpinner, self).__init__(**kwargs)
        self.category = kwargs.get('category', None)
        self.values = []
     
    def on_channels_updated(self, channels):
        print("on channels updated")
        self.values = channels.getNamesList(self.category)

class AccelMappingSpinner(Spinner):
    def __init__(self, **kwargs):
        super(AccelMappingSpinner, self).__init__(**kwargs)
        self.values = ['X', 'Y', 'Z']

class GyroMappingSpinner(Spinner):
    def __init__(self, **kwargs):
        super(GyroMappingSpinner, self).__init__(**kwargs)
        self.values = ['Yaw']

class SampleRateSpinner(Spinner):
    def __init__(self, **kwargs):
        super(SampleRateSpinner, self).__init__(**kwargs)
        self.values = ['Disabled', '1 Hz', '5 Hz', '10 Hz', '25 Hz', '50 Hz', '100 Hz']


class SampleRateSelectorView(BoxLayout):
    pass

class ChannelNameSelectorView(BoxLayout):
    def setValue(self, value):
        print(value)
        self.ids.channelName.text = value

class SplashView(BoxLayout):
    pass

class GPSChannelsView(BoxLayout):
    pass

class TargetConfigView(GridLayout):
    pass

class TrackConfigView(BoxLayout):
    pass
    
class CellTelemetryView(BoxLayout):
    pass

class BluetoothTelemetryView(BoxLayout):
    pass

class LuaScriptingView(BoxLayout):
    pass

class GPIOChannelsView(BoxLayout):
    def __init__(self, **kwargs):
        super(GPIOChannelsView, self).__init__(**kwargs)
        accordion = Accordion(orientation='vertical', size_hint=(1.0, None), height=90 * 3)
    
        # add button into that grid
        for i in range(3):
            channel = AccordionItem(title='Digital Input/Output ' + str(i + 1))
            editor = GPIOChannel()
            channel.add_widget(editor)
            accordion.add_widget(channel)
    
        #create a scroll view, with a size < size of the grid
        sv = ScrollView(size_hint=(1.0,1.0), do_scroll_x=False)
        sv.add_widget(accordion)
        self.add_widget(sv)
    
class AnalogPulseOutputChannelsView(BoxLayout):
    def __init__(self, **kwargs):
        super(AnalogPulseOutputChannelsView, self).__init__(**kwargs)
        accordion = Accordion(orientation='vertical', size_hint=(1.0, None), height=90 * 3)
    
        # add button into that grid
        for i in range(3):
            channel = AccordionItem(title='Pulse / Analog Output ' + str(i + 1))
            editor = AnalogPulseOutputChannel()
            channel.add_widget(editor)
            accordion.add_widget(channel)
    
        #create a scroll view, with a size < size of the grid
        sv = ScrollView(size_hint=(1.0,1.0), do_scroll_x=False)
        sv.add_widget(accordion)
        self.add_widget(sv)

class CANChannelsView(BoxLayout):
    pass

class OBD2ChannelsView(BoxLayout):
    pass
        
class PulseChannelsView(BoxLayout):
    def __init__(self, **kwargs):
        super(PulseChannelsView, self).__init__(**kwargs)
        accordion = Accordion(orientation='vertical', size_hint=(1.0, None), height=110 * 3)
    
        # add button into that grid
        for i in range(3):
            channel = AccordionItem(title='Pulse Input ' + str(i + 1))
            editor = PulseChannel()
            channel.add_widget(editor)
            accordion.add_widget(channel)
    
        #create a scroll view, with a size < size of the grid
        sv = ScrollView(size_hint=(1.0,1.0), do_scroll_x=False)
        sv.add_widget(accordion)
        self.add_widget(sv)


class AccelGyroChannelsView(BoxLayout):
    pass

class LinkedTreeViewLabel(TreeViewLabel):
    view = None
   
class LuaCodeInput(CodeInput):
    def __init__(self, **kwargs):
        super(LuaCodeInput, self).__init__(**kwargs)
        self.lexer= lexers.get_lexer_by_name('lua')

class LuaScriptingView(BoxLayout):
    #def __init__(self, **kwargs):
     #   print("init")
        
    def readScript(self):
        print("read script")

    def writeScript(self):
        print("write script")

    def runScript(self):
        print("run script")

class ToolbarView(BoxLayout):

    def __init__(self, **kwargs):
        self.register_event_type('on_read_config')
        super(ToolbarView, self).__init__(**kwargs)
        self.rcp = kwargs.get('rcp', None)
        self.app = kwargs.get('app', None)

    def readConfig(self):
        self.dispatch('on_read_config', None)

    def on_read_config(self, instance, *args):
        pass

class ToolbarButton(Button):
    pass

class RaceCaptureApp(App):
    def __init__(self, **kwargs):
        self.register_event_type('on_config_updated')
        self.register_event_type('on_channels_updated')
        super(RaceCaptureApp, self).__init__(**kwargs)

        #RaceCapture serial I/O 
        self.rcp = RcpSerial()
        
        #Central configuration object
        self.rcpConfig  = RcpConfig()

        #List of Channels
        self.channels = Channels()
        
        #List of config views
        self.configViews = []

    def on_read_config(self, instance, *args):
        config = self.rcp.getAnalogCfg(None)
        self.rcpConfig.fromJson(config)
        self.dispatch('on_config_updated', self.rcpConfig)

    def on_config_updated(self, rcpConfig):
        print('on config updated')
        self.analogChannelsView.dispatch('on_config_updated', rcpConfig)

    def updateConfig(self, json):
        self.rcpConfig.fromJson(json)
#        self.analogChannelsView.update(self.config)    
        print("updateConfig")

    def on_channels_updated(self, channels):
        for view in self.configViews:
            channelWidgets = list(kvquery(view, __class__=ChannelNameSpinner))
            for channelWidget in channelWidgets:
                print('dispatching channel updated')
                channelWidget.dispatch('on_channels_updated', channels)

    def build(self):
        def create_tree(text):
            return tree.add_node(LinkedTreeViewLabel(text=text, is_open=True, no_selection=True))

        def attach_node(text, n, view):
            label = LinkedTreeViewLabel(text=text)
            label.view = view
            label.color_selected =   [1.0,0,0,0.6]
            tree.add_node(label, n)
            self.configViews.append(view)

        def createConfigViews(tree):
            n = create_tree('Channels')
            attach_node('GPS', n, GPSChannelsView())
            attach_node('Track Channels', n, TrackConfigView())
            attach_node('Analog Inputs', n, AnalogChannelsView(channels=8))
            attach_node('Pulse Inputs', n, PulseChannelsView())
            attach_node('Digital Input/Outputs', n, GPIOChannelsView())
            attach_node('Accelerometer / Gyro', n, AccelGyroChannelsView())
            attach_node('Pulse / Analog Outputs', n, AnalogPulseOutputChannelsView())
            n = create_tree('CAN bus')
            attach_node('CAN Channels', n, CANChannelsView())
            attach_node('OBD2 Channels', n, OBD2ChannelsView())
            n = create_tree('Telemetry')
            attach_node('Cellular Telemetry', n, CellTelemetryView())
            attach_node('Bluetooth Link', n, BluetoothTelemetryView())
            n = create_tree('Scripting / Logging')
            attach_node('Lua Script', n, LuaScriptingView())

        def on_select_node(instance, value):
            # ensure that any keyboard is released
            self.content.get_parent_window().release_keyboard()

            try:
                self.content.clear_widgets()
                self.content.add_widget(value.view)
            except Exception, e:
                print e

        tree = TreeView(size_hint=(None, 1), width=200, hide_root=True, indent_level=0)
        tree.bind(selected_node=on_select_node)
        createConfigViews(tree)

        main = BoxLayout(orientation = 'horizontal', size_hint=(1.0, 0.95))
        main.add_widget(tree)
        main.add_widget(content)

        toolbar = ToolbarView(size_hint=(None, 0.05), rcp=self.rcp, app=self)
        toolbar.bind(on_read_config=self.on_read_config)
        
        self.content = SplashView()
        self.tree = tree
        self.toolbar = toolbar

        outer = BoxLayout(orientation='vertical', 
                            padding=5, spacing=5, size=(1024,576), 
                            size_hint=(None, None), pos_hint={'center_x': .5, 'center_y': .5})

        outer.add_widget(toolbar)
        outer.add_widget(main)

        self.dispatch('on_channels_updated', self.channels)

        return outer

if __name__ == '__main__':

    RaceCaptureApp().run()