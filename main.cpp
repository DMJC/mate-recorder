#include <gtkmm.h>
#include <portaudio.h>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>

// Suppress ALSA error messages
extern "C" {
    #include <alsa/asoundlib.h>
}

static void alsa_error_handler(const char *file, int line, 
                               const char *function, int err, 
                               const char *fmt, ...) {
    // Silently ignore ALSA errors
}

class AudioApp : public Gtk::Window {
public:
    AudioApp();
    virtual ~AudioApp();

protected:
    void on_menu_file_new();
    void on_menu_file_open();
    void on_menu_file_save();
    void on_menu_file_saveas();
    void on_menu_file_revert();
    void on_menu_file_properties();
    void on_menu_file_exit();
    
    void on_menu_edit_copy();
    void on_menu_edit_paste_insert();
    void on_menu_edit_paste_mix();
    void on_menu_edit_insert_file();
    void on_menu_edit_mix_with_file();
    void on_menu_edit_delete_before();
    void on_menu_edit_delete_after();
    void on_menu_edit_audio_properties();
    
    void on_menu_effects_increase_volume();
    void on_menu_effects_decrease_volume();
    void on_menu_effects_increase_speed();
    void on_menu_effects_decrease_speed();
    void on_menu_effects_add_echo();
    void on_menu_effects_reverse();
    
    void on_menu_help_about();
    
    void on_button_rewind();
    void on_button_fast_forward();
    void on_button_play();
    void on_button_stop();
    void on_button_record();
    
	bool on_waveform_draw(const Cairo::RefPtr<Cairo::Context>& cr);
    bool update_position();
    
    static int paCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData);

private:
    Gtk::Box m_VBox;
    
    Gtk::MenuBar m_MenuBar;
    Gtk::MenuItem m_MenuItemFile;
    Gtk::MenuItem m_MenuItemEdit;
    Gtk::MenuItem m_MenuItemEffects;
    Gtk::MenuItem m_MenuItemHelp;
    
    Gtk::Menu m_MenuFile;
    Gtk::Menu m_MenuEdit;
    Gtk::Menu m_MenuEffects;
    Gtk::Menu m_MenuHelp;
    
    Gtk::Box m_TopDisplayBox;
    Gtk::Frame m_PositionFrame;
    Gtk::Frame m_LengthFrame;
    Gtk::Label m_PositionLabel;
    Gtk::Label m_LengthLabel;
    
    Gtk::Frame m_WaveformFrame;
    Gtk::DrawingArea m_WaveformArea;
    
    Gtk::Box m_ControlBox;
    Gtk::Button m_ButtonRewind;
    Gtk::Button m_ButtonFastForward;
    Gtk::Button m_ButtonPlay;
    Gtk::Button m_ButtonStop;
    Gtk::Button m_ButtonRecord;
    Gtk::Scale* m_PositionScale;
    
    std::vector<float> m_AudioBuffer;
    std::vector<float> m_ClipboardBuffer;
    size_t m_CurrentPosition;
    size_t m_PlaybackPosition;
    bool m_IsPlaying;
    bool m_IsRecording;
    
    int m_SampleRate;
    int m_Channels;
    std::string m_CurrentFile;
    
    PaStream *m_Stream;
    sigc::connection m_TimerConnection;
    
    void init_audio();
    void cleanup_audio();
    void load_audio_file(const std::string& filename);
    void save_audio_file(const std::string& filename);
    void update_displays();
    void draw_waveform(const Cairo::RefPtr<Cairo::Context>& cr);
    std::string format_time(double seconds);
};

AudioApp::AudioApp()
    : m_VBox(Gtk::ORIENTATION_VERTICAL),
      m_TopDisplayBox(Gtk::ORIENTATION_HORIZONTAL),
      m_ControlBox(Gtk::ORIENTATION_HORIZONTAL),
      m_ButtonRewind("<<"),
      m_ButtonFastForward(">>"),
      m_ButtonPlay(">"),
      m_ButtonStop("■"),
      m_ButtonRecord("●"),
      m_CurrentPosition(0),
      m_PlaybackPosition(0),
      m_IsPlaying(false),
      m_IsRecording(false),
      m_SampleRate(44100),
      m_Channels(2),
      m_Stream(nullptr)
{
    set_title("Sound - Sound Recorder");
    set_default_size(400, 200);
    set_border_width(5);
    set_resizable(true);
    
    init_audio();
    
    Gtk::MenuItem* item;
    
    // File Menu
    item = Gtk::manage(new Gtk::MenuItem("New"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_file_new));
    m_MenuFile.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Open"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_file_open));
    m_MenuFile.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Save"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_file_save));
    m_MenuFile.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Save As"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_file_saveas));
    m_MenuFile.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Revert"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_file_revert));
    m_MenuFile.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Properties"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_file_properties));
    m_MenuFile.append(*item);
    
    m_MenuFile.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    
    item = Gtk::manage(new Gtk::MenuItem("Exit"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_file_exit));
    m_MenuFile.append(*item);
    
    m_MenuItemFile.set_label("File");
    m_MenuItemFile.set_submenu(m_MenuFile);
    m_MenuBar.append(m_MenuItemFile);
    
    // Edit Menu
    item = Gtk::manage(new Gtk::MenuItem("Copy"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_edit_copy));
    m_MenuEdit.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Paste Insert"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_edit_paste_insert));
    m_MenuEdit.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Paste Mix"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_edit_paste_mix));
    m_MenuEdit.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Insert File"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_edit_insert_file));
    m_MenuEdit.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Mix with File"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_edit_mix_with_file));
    m_MenuEdit.append(*item);
    
    m_MenuEdit.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    
    item = Gtk::manage(new Gtk::MenuItem("Delete Before Current Position"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_edit_delete_before));
    m_MenuEdit.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Delete After Current Position"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_edit_delete_after));
    m_MenuEdit.append(*item);
    
    m_MenuEdit.append(*Gtk::manage(new Gtk::SeparatorMenuItem()));
    
    item = Gtk::manage(new Gtk::MenuItem("Audio Properties"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_edit_audio_properties));
    m_MenuEdit.append(*item);
    
    m_MenuItemEdit.set_label("Edit");
    m_MenuItemEdit.set_submenu(m_MenuEdit);
    m_MenuBar.append(m_MenuItemEdit);
    
    // Effects Menu
    item = Gtk::manage(new Gtk::MenuItem("Increase Volume (by 25%)"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_effects_increase_volume));
    m_MenuEffects.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Decrease Volume"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_effects_decrease_volume));
    m_MenuEffects.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Increase Speed (by 100%)"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_effects_increase_speed));
    m_MenuEffects.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Decrease Speed"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_effects_decrease_speed));
    m_MenuEffects.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Add Echo"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_effects_add_echo));
    m_MenuEffects.append(*item);
    
    item = Gtk::manage(new Gtk::MenuItem("Reverse"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_effects_reverse));
    m_MenuEffects.append(*item);
    
    m_MenuItemEffects.set_label("Effects");
    m_MenuItemEffects.set_submenu(m_MenuEffects);
    m_MenuBar.append(m_MenuItemEffects);
    
    // Help Menu
    item = Gtk::manage(new Gtk::MenuItem("About"));
    item->signal_activate().connect(sigc::mem_fun(*this, &AudioApp::on_menu_help_about));
    m_MenuHelp.append(*item);
    
    m_MenuItemHelp.set_label("Help");
    m_MenuItemHelp.set_submenu(m_MenuHelp);
    m_MenuBar.append(m_MenuItemHelp);
    
    // Setup Position label
    m_PositionLabel.set_text("0.00 sec");
    m_PositionLabel.set_size_request(70, -1);
    
    m_PositionFrame.set_label("Position:");
    m_PositionFrame.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
    m_PositionFrame.add(m_PositionLabel);
    
    // Setup Length label
    m_LengthLabel.set_text("0.00 sec");
    m_LengthLabel.set_size_request(70, -1);
    
    m_LengthFrame.set_label("Length:");
    m_LengthFrame.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
    m_LengthFrame.add(m_LengthLabel);
    
	// --- Waveform ---
	m_WaveformArea.set_size_request(112, 35);

	// Make it behave nicely in boxes (horizontal stretch ok; vertical fixed-ish)
	m_WaveformArea.set_hexpand(true);
	m_WaveformArea.set_vexpand(false);

	m_WaveformArea.signal_draw().connect(sigc::mem_fun(*this, &AudioApp::on_waveform_draw), false);

	m_WaveformFrame.add(m_WaveformArea);
	m_WaveformFrame.set_shadow_type(Gtk::SHADOW_IN);

	// Frame should stretch horizontally but not vertically
	m_WaveformFrame.set_hexpand(true);
	m_WaveformFrame.set_vexpand(false);

	// --- Top display row ---
	m_TopDisplayBox.set_spacing(5);
	m_TopDisplayBox.set_hexpand(true);
	m_TopDisplayBox.set_vexpand(false);
	
	m_TopDisplayBox.pack_start(m_PositionFrame, false, false, 0);
	m_TopDisplayBox.pack_start(m_WaveformFrame, true,  true,  0);  // allow 	horizontal stretch
	m_TopDisplayBox.pack_start(m_LengthFrame,   false, false, 0);
	
	// --- Menu bar ---
	m_MenuBar.set_hexpand(true);
	m_MenuBar.set_vexpand(false);

	// --- Position scale (use the member, not a local) ---
	m_PositionScale = Gtk::manage(new 	Gtk::Scale(Gtk::ORIENTATION_HORIZONTAL));
	m_PositionScale->set_range(0, 100);
	m_PositionScale->set_value(0);
	m_PositionScale->set_draw_value(false);
	m_PositionScale->set_hexpand(true);
	m_PositionScale->set_vexpand(false);

	// --- Controls row ---
 	m_ControlBox.pack_start(m_ButtonRewind, true, true, 0);
	m_ControlBox.pack_start(m_ButtonFastForward, true, true, 0);
	m_ControlBox.pack_start(m_ButtonPlay, true, true, 0);
	m_ControlBox.pack_start(m_ButtonStop, true, true, 0);
	m_ControlBox.pack_start(m_ButtonRecord, true, true, 0);

	m_ControlBox.set_spacing(3);
	m_ControlBox.set_hexpand(true);
	m_ControlBox.set_vexpand(false);
	
	// --- Pack into main VBox ---
	m_VBox.set_spacing(5);
	m_VBox.set_hexpand(true);
	m_VBox.set_vexpand(true);

	// Row 1
	m_VBox.pack_start(m_MenuBar, false, false, 0);

	// Row 2
	m_VBox.pack_start(m_TopDisplayBox, false, false, 0);

	// Row 3
	m_VBox.pack_start(*m_PositionScale, false, false, 0);

	// Row 4
	m_VBox.pack_start(m_ControlBox, false, false, 0);

	// Finally
	add(m_VBox);
	show_all_children();

    
    m_TimerConnection = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &AudioApp::update_position), 50);
}


AudioApp::~AudioApp() {
    m_TimerConnection.disconnect();
    cleanup_audio();
}

void AudioApp::init_audio() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }
}

void AudioApp::cleanup_audio() {
    if (m_Stream) {
        Pa_CloseStream(m_Stream);
        m_Stream = nullptr;
    }
    Pa_Terminate();
}

int AudioApp::paCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData)
{
    AudioApp *app = (AudioApp*)userData;
    float *out = (float*)outputBuffer;
    const float *in = (const float*)inputBuffer;
    
    if (app->m_IsRecording && in) {
        for (unsigned long i = 0; i < framesPerBuffer * app->m_Channels; i++) {
            app->m_AudioBuffer.push_back(in[i]);
        }
    }
    
    if (app->m_IsPlaying && out) {
        for (unsigned long i = 0; i < framesPerBuffer * app->m_Channels; i++) {
            if (app->m_PlaybackPosition < app->m_AudioBuffer.size()) {
                out[i] = app->m_AudioBuffer[app->m_PlaybackPosition++];
            } else {
                out[i] = 0.0f;
                app->m_IsPlaying = false;
            }
        }
    } else if (out) {
        memset(out, 0, framesPerBuffer * app->m_Channels * sizeof(float));
    }
    
    return paContinue;
}

bool AudioApp::on_waveform_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    draw_waveform(cr);
    return true; // handled for the waveform area only
}

void AudioApp::draw_waveform(const Cairo::RefPtr<Cairo::Context>& cr) {
    cr->save();

    auto allocation = m_WaveformArea.get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();

    cr->rectangle(0, 0, width, height);
    cr->clip();

    cr->set_source_rgb(0.0, 0.0, 0.0);
    cr->paint();

    if (!m_AudioBuffer.empty()) {
        cr->set_source_rgb(0.0, 1.0, 0.0);
        cr->set_line_width(1.0);

        const int centerY = height / 2;
        const size_t samplesPerPixel = std::max<size_t>(1, m_AudioBuffer.size() / std::max(1, width));

        for (int x = 0; x < width; x++) {
            size_t startSample = (size_t)x * samplesPerPixel;
            size_t endSample = std::min(startSample + samplesPerPixel, m_AudioBuffer.size());

            float maxVal = 0.0f;
            float minVal = 0.0f;

            for (size_t i = startSample; i < endSample; i += m_Channels) {
                float val = m_AudioBuffer[i];
                maxVal = std::max(maxVal, val);
                minVal = std::min(minVal, val);
            }

            int y1 = centerY - (int)(maxVal * centerY * 0.9f);
            int y2 = centerY - (int)(minVal * centerY * 0.9f);

            cr->move_to(x + 0.5, y1 + 0.5);
            cr->line_to(x + 0.5, y2 + 0.5);
            cr->stroke();
        }
    }

    cr->restore();
}

bool AudioApp::update_position() {
    update_displays();
    m_WaveformArea.queue_draw();
    return true;
}

void AudioApp::update_displays() {
    double posSeconds = (double)m_PlaybackPosition / (m_SampleRate * m_Channels);
    double lenSeconds = (double)m_AudioBuffer.size() / (m_SampleRate * m_Channels);
    
    m_PositionLabel.set_text(format_time(posSeconds));
    m_LengthLabel.set_text(format_time(lenSeconds));
}

std::string AudioApp::format_time(double seconds) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.2f sec", seconds);
    return std::string(buffer);
}

void AudioApp::on_menu_file_new() {
    m_AudioBuffer.clear();
    m_CurrentPosition = 0;
    m_PlaybackPosition = 0;
    m_CurrentFile.clear();
    update_displays();
    m_WaveformArea.queue_draw();
}

void AudioApp::on_menu_file_open() {
    Gtk::FileChooserDialog dialog("Open Audio File", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
    
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Audio files");
    filter->add_mime_type("audio/wav");
    filter->add_pattern("*.wav");
    dialog.add_filter(filter);
    
    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        load_audio_file(dialog.get_filename());
    }
}

void AudioApp::on_menu_file_save() {
    if (m_CurrentFile.empty()) {
        on_menu_file_saveas();
    } else {
        save_audio_file(m_CurrentFile);
    }
}

void AudioApp::on_menu_file_saveas() {
    Gtk::FileChooserDialog dialog("Save Audio File", Gtk::FILE_CHOOSER_ACTION_SAVE);
    dialog.set_transient_for(*this);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Save", Gtk::RESPONSE_OK);
    
    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        m_CurrentFile = dialog.get_filename();
        save_audio_file(m_CurrentFile);
    }
}

void AudioApp::on_menu_file_revert() {
    if (!m_CurrentFile.empty()) {
        load_audio_file(m_CurrentFile);
    }
}

void AudioApp::on_menu_file_properties() {
    Gtk::MessageDialog dialog(*this, "Audio Properties", false, Gtk::MESSAGE_INFO);
    char info[256];
    snprintf(info, sizeof(info), "Sample Rate: %d Hz\nChannels: %d\nSamples: %zu",
             m_SampleRate, m_Channels, m_AudioBuffer.size());
    dialog.set_secondary_text(info);
    dialog.run();
}

void AudioApp::on_menu_file_exit() {
    hide();
}

void AudioApp::on_menu_edit_copy() {
    if (m_CurrentPosition < m_AudioBuffer.size()) {
        m_ClipboardBuffer.assign(m_AudioBuffer.begin() + m_CurrentPosition, 
                                 m_AudioBuffer.end());
    }
}

void AudioApp::on_menu_edit_paste_insert() {
    if (!m_ClipboardBuffer.empty() && m_CurrentPosition <= m_AudioBuffer.size()) {
        m_AudioBuffer.insert(m_AudioBuffer.begin() + m_CurrentPosition,
                            m_ClipboardBuffer.begin(), m_ClipboardBuffer.end());
        update_displays();
    }
}

void AudioApp::on_menu_edit_paste_mix() {
    if (!m_ClipboardBuffer.empty() && m_CurrentPosition < m_AudioBuffer.size()) {
        for (size_t i = 0; i < m_ClipboardBuffer.size() && 
             (m_CurrentPosition + i) < m_AudioBuffer.size(); i++) {
            m_AudioBuffer[m_CurrentPosition + i] = 
                (m_AudioBuffer[m_CurrentPosition + i] + m_ClipboardBuffer[i]) * 0.5f;
        }
        update_displays();
    }
}

void AudioApp::on_menu_edit_insert_file() {
    Gtk::FileChooserDialog dialog("Insert Audio File", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*this);
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);
    
    int result = dialog.run();
    if (result == Gtk::RESPONSE_OK) {
        std::vector<float> tempBuffer;
        SF_INFO sfinfo;
        SNDFILE* file = sf_open(dialog.get_filename().c_str(), SFM_READ, &sfinfo);
        if (file) {
            tempBuffer.resize(sfinfo.frames * sfinfo.channels);
            sf_read_float(file, tempBuffer.data(), tempBuffer.size());
            sf_close(file);
            
            m_AudioBuffer.insert(m_AudioBuffer.begin() + m_CurrentPosition,
                                tempBuffer.begin(), tempBuffer.end());
            update_displays();
        }
    }
}

void AudioApp::on_menu_edit_mix_with_file() {
    on_menu_edit_insert_file();
}

void AudioApp::on_menu_edit_delete_before() {
    if (m_CurrentPosition > 0) {
        m_AudioBuffer.erase(m_AudioBuffer.begin(), 
                           m_AudioBuffer.begin() + m_CurrentPosition);
        m_CurrentPosition = 0;
        m_PlaybackPosition = 0;
        update_displays();
    }
}

void AudioApp::on_menu_edit_delete_after() {
    if (m_CurrentPosition < m_AudioBuffer.size()) {
        m_AudioBuffer.erase(m_AudioBuffer.begin() + m_CurrentPosition, 
                           m_AudioBuffer.end());
        update_displays();
    }
}

void AudioApp::on_menu_edit_audio_properties() {
    on_menu_file_properties();
}

void AudioApp::on_menu_effects_increase_volume() {
    for (auto& sample : m_AudioBuffer) {
        sample *= 1.25f;
        sample = std::max(-1.0f, std::min(1.0f, sample));
    }
    m_WaveformArea.queue_draw();
}

void AudioApp::on_menu_effects_decrease_volume() {
    for (auto& sample : m_AudioBuffer) {
        sample *= 0.8f;
    }
    m_WaveformArea.queue_draw();
}

void AudioApp::on_menu_effects_increase_speed() {
    std::vector<float> newBuffer;
    newBuffer.reserve(m_AudioBuffer.size() / 2);
    for (size_t i = 0; i < m_AudioBuffer.size(); i += 2 * m_Channels) {
        for (int c = 0; c < m_Channels && (i + c) < m_AudioBuffer.size(); c++) {
            newBuffer.push_back(m_AudioBuffer[i + c]);
        }
    }
    m_AudioBuffer = newBuffer;
    update_displays();
}

void AudioApp::on_menu_effects_decrease_speed() {
    std::vector<float> newBuffer;
    newBuffer.reserve(m_AudioBuffer.size() * 2);
    for (size_t i = 0; i < m_AudioBuffer.size(); i += m_Channels) {
        for (int c = 0; c < m_Channels && (i + c) < m_AudioBuffer.size(); c++) {
            newBuffer.push_back(m_AudioBuffer[i + c]);
            newBuffer.push_back(m_AudioBuffer[i + c]);
        }
    }
    m_AudioBuffer = newBuffer;
    update_displays();
}

void AudioApp::on_menu_effects_add_echo() {
    int echoDelay = m_SampleRate / 2;
    std::vector<float> newBuffer = m_AudioBuffer;
    
    for (size_t i = echoDelay * m_Channels; i < m_AudioBuffer.size(); i++) {
        newBuffer[i] = m_AudioBuffer[i] + m_AudioBuffer[i - echoDelay * m_Channels] * 0.5f;
        newBuffer[i] = std::max(-1.0f, std::min(1.0f, newBuffer[i]));
    }
    m_AudioBuffer = newBuffer;
    m_WaveformArea.queue_draw();
}

void AudioApp::on_menu_effects_reverse() {
    std::reverse(m_AudioBuffer.begin(), m_AudioBuffer.end());
    m_WaveformArea.queue_draw();
}

void AudioApp::on_menu_help_about() {
    Gtk::MessageDialog dialog(*this, "About Sound Recorder");
    dialog.set_secondary_text("GTK3 Audio Recording and Mixing Application\nVersion 1.0");
    dialog.run();
}

void AudioApp::on_button_rewind() {
    m_PlaybackPosition = 0;
    m_CurrentPosition = 0;
    update_displays();
}

void AudioApp::on_button_fast_forward() {
    m_PlaybackPosition = m_AudioBuffer.size();
    m_CurrentPosition = m_AudioBuffer.size();
    update_displays();
}

void AudioApp::on_button_play() {
    if (m_AudioBuffer.empty()) return;
    
    m_IsPlaying = true;
    m_IsRecording = false;
    
    if (!m_Stream) {
        PaError err = Pa_OpenDefaultStream(&m_Stream, 0, m_Channels, paFloat32,
                                          m_SampleRate, 256, paCallback, this);
        if (err == paNoError) {
            Pa_StartStream(m_Stream);
        }
    }
}

void AudioApp::on_button_stop() {
    m_IsPlaying = false;
    m_IsRecording = false;
    
    if (m_Stream) {
        Pa_StopStream(m_Stream);
        Pa_CloseStream(m_Stream);
        m_Stream = nullptr;
    }
}

void AudioApp::on_button_record() {
    m_IsRecording = true;
    m_IsPlaying = false;
    m_AudioBuffer.clear();
    
    if (!m_Stream) {
        PaError err = Pa_OpenDefaultStream(&m_Stream, m_Channels, 0, paFloat32,
                                          m_SampleRate, 256, paCallback, this);
        if (err == paNoError) {
            Pa_StartStream(m_Stream);
        }
    }
}

void AudioApp::load_audio_file(const std::string& filename) {
    SF_INFO sfinfo;
    SNDFILE* file = sf_open(filename.c_str(), SFM_READ, &sfinfo);
    
    if (!file) {
        Gtk::MessageDialog dialog(*this, "Error opening file", false, Gtk::MESSAGE_ERROR);
        dialog.run();
        return;
    }
    
    m_SampleRate = sfinfo.samplerate;
    m_Channels = sfinfo.channels;
    m_AudioBuffer.resize(sfinfo.frames * sfinfo.channels);
    
    sf_read_float(file, m_AudioBuffer.data(), m_AudioBuffer.size());
    sf_close(file);
    
    m_CurrentFile = filename;
    m_CurrentPosition = 0;
    m_PlaybackPosition = 0;
    update_displays();
    m_WaveformArea.queue_draw();
}

void AudioApp::save_audio_file(const std::string& filename) {
    SF_INFO sfinfo;
    sfinfo.samplerate = m_SampleRate;
    sfinfo.channels = m_Channels;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    
    SNDFILE* file = sf_open(filename.c_str(), SFM_WRITE, &sfinfo);
    
    if (!file) {
        Gtk::MessageDialog dialog(*this, "Error saving file", false, Gtk::MESSAGE_ERROR);
        dialog.run();
        return;
    }
    
    sf_write_float(file, m_AudioBuffer.data(), m_AudioBuffer.size());
    sf_close(file);
}

int main(int argc, char* argv[]) {
    // Suppress ALSA error messages
    snd_lib_error_set_handler(alsa_error_handler);
    
    auto app = Gtk::Application::create(argc, argv, "org.soundrecorder.app");
    
    AudioApp window;
    
    return app->run(window);
}
