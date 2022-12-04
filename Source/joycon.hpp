#include "JuceHeader.h"
#include "hidapi.h"

class Joycon
{
public:
    Joycon() :
    isLeft(false), hid_dev(nullptr), imu_enabled(true), do_localize(true),
    filterweight(0.05f), rumble_obj(160, 320, 0), pollThread(*this)
    {
    }

	Joycon(hid_device* dev, bool imu, bool localize, float alpha, bool left) :
    isLeft(left), hid_dev(dev), imu_enabled(imu), do_localize (localize),
    filterweight(alpha), rumble_obj(160, 320, 0), pollThread(*this)
    {
    }

    void Begin()
    {
        pollThread.startThread(juce::Thread::Priority::normal);
    }

    void SetRumble(float low_freq, float high_freq, float amp, uint time = 0)
    {
        if (state <= state_::ATTACHED)
        {
            return;
        }

		if (rumble_obj.isRumbleOn() == false)
        {
            rumble_obj.setVals(low_freq, high_freq, amp, time);
        }
    }

	bool Attach(uint8_t leds_ = 0x0)
    {
        state = state_::ATTACHED;

        /* set input report mode, simple push on button press */
        // Subcommand(0x3, {0x3f}, false);

        dumpCalibrationData();

        /* pairing info */
        // Subcommand(0x1, {0x01}, 1);
        // Subcommand(0x1, {0x02}, 1);
        // Subcommand(0x1, {0x03}, 1);

        /* set player leds */
        Subcommand(0x30, {leds_}, 1);

        /* enable imu sensor */
        Subcommand(0x40, {(imu_enabled ? (uint8_t)0x1 : (uint8_t)0x0)}, true);

        /* set input report mode, full 60hz */
        Subcommand(0x3, {0x30}, true);

        /* set vibration enable, on */
        Subcommand(0x48, {0x1}, true);

        pitchRollYaw.x = 0;
        pitchRollYaw.y = 0;
        pitchRollYaw.z = 0;

        DebugPrint("Done with init.", DebugType::COMMS);

        return true;
    }

    void SetFilterCoeff(float a)
    {
        filterweight = a;
    }

    void Detach()
    {
        stop_polling = true;
        PrintArray(max, DebugType::IMU);
        PrintArray(sum, DebugType::IMU);

        if (state > state_::NO_JOYCONS)
        {
            Subcommand(0x30, std::vector<uint8_t> { 0x0 });
            Subcommand(0x40, std::vector<uint8_t> { 0x0 });
            Subcommand(0x48, std::vector<uint8_t> { 0x0 });
            Subcommand(0x3, std::vector<uint8_t> { 0x3f });
        }

        if (state > state_::DROPPED)
        {
            hid_close(hid_dev);
        }

        state = state_::NOT_ATTACHED;
    }

    void Update()
    {
        if (state > state_::NO_JOYCONS)
        {
            juce::CriticalSection critical;
            juce::ScopedLock scoped_lock(critical);

            std::vector<uint8_t> report_buf(report_len);

            while (!reports.empty())
            {
                Report rep = reports.front();

                rep.CopyBuffer(report_buf);

                if (imu_enabled)
                {
                    if (do_localize)
                    {
                        ProcessIMU(report_buf);
                    }
                    else
                    {
                        ExtractIMUValues(report_buf, 0);
                    }
                }

                if (ts_de == report_buf[1])
                {
                    std::stringstream ss;
                    ss << "Duplicate timestamp dequeued. TS: ";
                    ss << std::hex << std::setfill('0') << std::setw(2) << ts_de;
                    DebugPrint(ss.str(), DebugType::THREADING);
                }

                std::stringstream ss;
                ts_de = report_buf[1];
                ss << "Dequeue. Queue length: " << reports.size();
                ss << ". Packet ID: " << std::hex << std::setfill('0') << std::setw(2) << report_buf[0];
                ss << ". Timestamp: " << std::hex << std::setfill('0') << std::setw(2) << report_buf[1];
                ss << ". Lag to dequeue: " <<  (juce::Time::getCurrentTime() - rep.GetTime()).inMilliseconds();
                ss << ". Lag between packets (expect 15ms): ";
                ss << (juce::Time::getCurrentTime() - ts_prev).inMilliseconds();
                DebugPrint(ss.str(), DebugType::THREADING);
                ts_prev = rep.GetTime();
            }

            ProcessButtonsAndStick(report_buf);
        }
    }


    enum DebugType
    {
        NONE,
        ALL,
        COMMS,
        THREADING,
        IMU,
        RUMBLE,
    };

    enum state_
    {
        NOT_ATTACHED,
        DROPPED,
        NO_JOYCONS,
        ATTACHED,
        INPUT_MODE_0x30,
        IMU_DATA_OK,
    };

    enum Button
    {
        DPAD_DOWN = 0,
        DPAD_RIGHT = 1,
        DPAD_LEFT = 2,
        DPAD_UP = 3,
        SL = 4,
        SR = 5,
        MINUS = 6,
        HOME = 7,
        PLUS = 8,
        CAPTURE = 9,
        STICK = 10,
        SHOULDER_1 = 11,
        SHOULDER_2 = 12
    };

    static uint const vendor_id = 0x057e;
    static uint const product_id_left = 0x2006;
	static uint const product_id_right = 0x2007;

    bool isLeft;
    state_ state;

private:
    std::array<std::atomic<bool>, 13> buttons_down;
    std::array<std::atomic<bool>, 13> buttons_up;
    std::array<std::atomic<bool>, 13> buttons;
    std::array<std::atomic<bool>, 13> down_;

    hid_device* hid_dev;

    std::vector<float> stick = { 0, 0 };

    std::vector<uint8_t> default_buf = { 0x0, 0x1, 0x40, 0x40, 0x0, 0x1, 0x40, 0x40 };

    std::vector<uint8_t> stick_raw = { 0, 0, 0 };
    std::vector<uint16_t> stick_cal = { 0, 0, 0, 0, 0, 0 };
    uint16_t deadzone;
    std::vector<uint16_t> stick_precal = { 0, 0 };

    bool stop_polling = false;
    int timestamp;
    bool imu_enabled = false;

    juce::Vector3D<int16_t> acc_r = { 0, 0, 0 };
    juce::Vector3D<float> acc_g;

    juce::Vector3D<int16_t> gyr_neutral = { 0, 0, 0 };
    juce::Vector3D<int16_t> gyr_r = { 0, 0, 0 };
    juce::Vector3D<float> gyr_g;

    juce::Vector3D<float> pitchRollYaw;

    bool do_localize;
    float filterweight;

    uint report_len = 49;
    class Report
    {
    public:
        std::vector<uint8_t> r;
        juce::Time t;

        Report(std::vector<uint8_t> report, juce::Time time)
        {
            r = report;
            t = time;
        }

        juce::Time GetTime()
        {
            return t;
        }

        void CopyBuffer(std::vector<uint8_t> b)
        {
            b = r;
        }
    };

    class Rumble : private juce::Timer
    {
    public:
        Rumble(float low_freq, float high_freq, float amplitude, uint time_ms = 0)
        {
            setVals(low_freq, high_freq, amplitude, time_ms);
        }

        void setVals(float low_freq, float high_freq, float amplitude, uint time_ms = 0)
        {
            h_f = high_freq;
            amp = amplitude;
            l_f = low_freq;
            timed_rumble = false;

            if (time_ms)
            {
                timed_rumble = true;
                startTimer((int)time_ms);
            }
        }

        std::vector<uint8_t> GetData()
        {
            std::vector<uint8_t> rumble_data(8);

            if (amp == 0.0f)
            {
                rumble_data[0] = 0x0;
                rumble_data[1] = 0x1;
                rumble_data[2] = 0x40;
                rumble_data[3] = 0x40;
            }
            else
            {
                l_f = clamp(l_f, 40.875885f, 626.286133f);
                amp = clamp(amp, 0.0f, 1.0f);
                h_f = clamp(h_f, 81.75177f, 1252.572266f);

                uint16_t hf = (uint16_t)((std::roundf(32.f * std::log2f(h_f * 0.1f)) - 0x60) * 4);
                uint8_t lf = (uint8_t)(std::roundf(32.f * std::log2f(l_f * 0.1f)) - 0x40);

                uint8_t hf_amp;
                if (amp == 0) {
                    hf_amp = 0; }
                else
                if (amp < 0.117) {
                    hf_amp = (uint8_t)(((std::log2f(amp * 1000.f) * 32) - 0x60) / (5 - std::powf(amp, 2)) - 1); }
                else
                if (amp < 0.23) {
                    hf_amp = (uint8_t)(((std::log2f(amp * 1000.f) * 32) - 0x60) - 0x5c); }
                else {
                    hf_amp = (uint8_t)((((std::log2f(amp * 1000) * 32) - 0x60) * 2) - 0xf6); }

                uint16_t lf_amp = (uint16_t)(std::roundf(hf_amp) * .5);
                uint8_t parity = (uint8_t)(lf_amp % 2);

                if (parity > 0) {
                    --lf_amp; }

                lf_amp = (uint16_t)(lf_amp >> 1);
                lf_amp += 0x40;
                if (parity > 0) {
                    lf_amp |= 0x8000; }

                rumble_data[0] = (uint8_t)(hf & 0xff);
                rumble_data[1] = (uint8_t)((hf >> 8) & 0xff);
                rumble_data[2] = lf;
                rumble_data[1] += hf_amp;
                rumble_data[2] += (uint8_t)((lf_amp >> 8) & 0xff);
                rumble_data[3] += (uint8_t)(lf_amp & 0xff);
            }

            for (auto i = 0; i < 4; ++i)
            {
                rumble_data[4 + (size_t)i] = rumble_data[(size_t)i];
            }
            //Debug.Log(string.Format("Encoded hex freq: {0:X2}", encoded_hex_freq));
            //Debug.Log(string.Format("lf_amp: {0:X4}", lf_amp));
            //Debug.Log(string.Format("hf_amp: {0:X2}", hf_amp));
            //Debug.Log(string.Format("l_f: {0:F}", l_f));
            //Debug.Log(string.Format("hf: {0:X4}", hf));
            //Debug.Log(string.Format("lf: {0:X2}", lf));
            return rumble_data;
        }

        bool isRumbleOn()
        {
            return timed_rumble;
        }

    private:
        float h_f, amp, l_f;

        std::atomic<bool> timed_rumble;

        float clamp(float x, float _min, float _max)
        {
            if (x < _min) return _min;
            if (x > _max) return _max;
            return x;
        }

        void timerCallback()
        {
            timed_rumble = false;
        }
    };

    std::queue<Report> reports;
    Rumble rumble_obj;

    uint8_t global_count = 0;

    void DebugPrint(String s, DebugType d)
    {
        if (d != DebugType::NONE)
        {
            std::cout << s;
        }
    }

    bool GetButtonDown(Button b)
    {
        return buttons_down[b];
    }

    bool GetButton(Button b)
    {
        return buttons[b];
    }

    bool GetButtonUp(Button b)
    {
        return buttons_up[b];
    }

    std::vector<float> GetStick()
    {
        return stick;
    }

    juce::Vector3D<float> GetGyro()
    {
        return gyr_g;
    }

    juce::Vector3D<float> GetAccel()
    {
        return acc_g;
    }

    /* returns values in range [-1, 1] for each angle */
    juce::Vector3D<float> getPitchRollYaw()
    {
        return pitchRollYaw;
    }

    uint8_t ts_en;
    uint8_t ts_de;
    juce::Time ts_prev;

    int ReceiveRaw()
    {
        if (hid_dev == nullptr) return -2;

        hid_set_nonblocking(hid_dev, 0);

        std::vector<uint8_t> raw_buf(report_len);

        int ret = hid_read(hid_dev, raw_buf.data(), report_len);

        if (ret > 0)
        {
            juce::CriticalSection critical;
            juce::GenericScopedLock<juce::CriticalSection> scoped_lock(critical);

            reports.push(Report(raw_buf, juce::Time::getCurrentTime()));

            std::stringstream ss;
            if (ts_en == raw_buf[1])
            {
                ss << "Duplicate timestamp enqueued. TS: " << std::hex << std::setfill('0') << std::setw(2) << ts_en;
                DebugPrint(ss.str(), DebugType::THREADING);
            }
            ts_en = raw_buf[1];
            ss << "Enqueue. Bytes read: " << ret << ". Timestamp: 0x";
            ss << std::hex << std::setfill('0') << std::setw(2) << raw_buf[1];
            DebugPrint(ss.str(), DebugType::THREADING);
        }

        return ret;
    }

    class PollThreadObj : public juce::Thread
    {
    public:
        PollThreadObj(Joycon& parent) : juce::Thread("poll", 0), j(parent) {}

        void run() override
        {
            int attempts = 0;
            if (j.stop_polling || (j.state <= state_::NO_JOYCONS))
            {
                j.DebugPrint("End poll loop.", DebugType::THREADING);
                stopThread(50);
            }
            else
            {
                j.SendRumble(j.rumble_obj.GetData());

                // TODO: why twice?
                int a = j.ReceiveRaw();
                a = j.ReceiveRaw();

                if (a > 0)
                {
                    j.state = state_::IMU_DATA_OK;
                    attempts = 0;
                }
                else if (attempts > 1000)
                {
                    j.state = state_::DROPPED;
                    j.DebugPrint("Connection lost. Is the Joy-Con connected?", DebugType::ALL);
                }
                else
                {
                    j.DebugPrint("Pause 5ms", DebugType::THREADING);
                    sleep(5);
                }

                ++attempts;
            }
        }

        private:
            Joycon& j;
    };

    PollThreadObj pollThread;

    std::vector<float> max = { 0, 0, 0 };
    std::vector<float> sum = { 0, 0, 0 };

    int ProcessButtonsAndStick(std::vector<uint8_t> report_buf)
    {
        if (report_buf[0] == 0x00) return -1;

        stick_raw[0] = report_buf[6 + (isLeft ? 0 : 3)];
        stick_raw[1] = report_buf[7 + (isLeft ? 0 : 3)];
        stick_raw[2] = report_buf[8 + (isLeft ? 0 : 3)];

        stick_precal[0] = (uint16_t)(stick_raw[0] + ((stick_raw[1] & 0xf) << 8));
        stick_precal[1] = (uint16_t)((stick_raw[1] >> 4) + (stick_raw[2] << 4));
        stick = CenterSticks(stick_precal);

        for (size_t i = 0; i < buttons.size(); ++i)
        {
            down_[i] = buttons[i].load();
        }

        buttons[(int)Button::DPAD_DOWN] = (report_buf[3 + (isLeft ? 2 : 0)] & (isLeft ? 0x01 : 0x04)) != 0;
        buttons[(int)Button::DPAD_RIGHT] = (report_buf[3 + (isLeft ? 2 : 0)] & (isLeft ? 0x04 : 0x08)) != 0;
        buttons[(int)Button::DPAD_UP] = (report_buf[3 + (isLeft ? 2 : 0)] & (isLeft ? 0x02 : 0x02)) != 0;
        buttons[(int)Button::DPAD_LEFT] = (report_buf[3 + (isLeft ? 2 : 0)] & (isLeft ? 0x08 : 0x01)) != 0;
        buttons[(int)Button::HOME] = ((report_buf[4] & 0x10) != 0);
        buttons[(int)Button::MINUS] = ((report_buf[4] & 0x01) != 0);
        buttons[(int)Button::PLUS] = ((report_buf[4] & 0x02) != 0);
        buttons[(int)Button::STICK] = ((report_buf[4] & (isLeft ? 0x08 : 0x04)) != 0);
        buttons[(int)Button::SHOULDER_1] = (report_buf[3 + (isLeft ? 2 : 0)] & 0x40) != 0;
        buttons[(int)Button::SHOULDER_2] = (report_buf[3 + (isLeft ? 2 : 0)] & 0x80) != 0;
        buttons[(int)Button::SR] = (report_buf[3 + (isLeft ? 2 : 0)] & 0x10) != 0;
        buttons[(int)Button::SL] = (report_buf[3 + (isLeft ? 2 : 0)] & 0x20) != 0;

        for (size_t i = 0; i < buttons.size(); ++i)
        {
            buttons_up[i] = (down_[i] && !buttons[i]);
            buttons_down[i] = (!down_[i] && buttons[i]);
        }

        return 0;
    }

    void ExtractIMUValues(std::vector<uint8_t> report_buf, size_t n = 0)
    {
        gyr_r.x = (int16_t)((int16_t)report_buf[19 + n * 12] + ((report_buf[20 + n * 12] << 8) & 0xff00));
        gyr_r.y = (int16_t)((int16_t)report_buf[21 + n * 12] + ((report_buf[22 + n * 12] << 8) & 0xff00));
        gyr_r.z = (int16_t)((int16_t)report_buf[23 + n * 12] + ((report_buf[24 + n * 12] << 8) & 0xff00));
        acc_r.x = (int16_t)((int16_t)report_buf[13 + n * 12] + ((report_buf[14 + n * 12] << 8) & 0xff00));
        acc_r.y = (int16_t)((int16_t)report_buf[15 + n * 12] + ((report_buf[16 + n * 12] << 8) & 0xff00));
        acc_r.z = (int16_t)((int16_t)report_buf[17 + n * 12] + ((report_buf[18 + n * 12] << 8) & 0xff00));

        acc_g.x = acc_r.x * 0.000244f;
        gyr_g.x = (gyr_r.x - gyr_neutral.x) * 0.070f;
        if (std::abs(acc_g.x) > std::abs(max[0])) max[0] = acc_g.x;

        acc_g.y = acc_r.y * 0.000244f;
        gyr_g.y = (gyr_r.y - gyr_neutral.y) * 0.070f;
        if (std::abs(acc_g.y) > std::abs(max[1])) max[1] = acc_g.y;

        acc_g.z = acc_r.z * 0.000244f;
        gyr_g.z = (gyr_r.z - gyr_neutral.z) * 0.070f;
        if (std::abs(acc_g.z) > std::abs(max[2])) max[2] = acc_g.z;
    }

    int ProcessIMU(std::vector<uint8_t> report_buf)
    {
        if (!imu_enabled || state < state_::IMU_DATA_OK)
            return -1;

        if (report_buf[0] != 0x30) return -1; // no gyro data

        // read raw IMU values
        int dt = (report_buf[1] - timestamp);
        if (report_buf[1] < timestamp)
        {
            dt += 0x100;
        }

        for (size_t n = 0; n < 3; ++n)
        {
            ExtractIMUValues(report_buf, n);

			float dt_sec = 0.005f * dt;

            sum[0] += gyr_g.x * dt_sec;
            sum[1] += gyr_g.y * dt_sec;
            sum[2] += gyr_g.z * dt_sec;

            if (isLeft)
            {
                gyr_g.y *= -1;
                gyr_g.z *= -1;
                acc_g.y *= -1;
                acc_g.z *= -1;
            }

            // Calculating Roll and Pitch from the accelerometer data
            // TODO error correction
            auto accAngleX = std::atan(acc_g.x / std::sqrt(pow(acc_g.y, 2) + std::pow(acc_g.z, 2))) * 180.f / juce::MathConstants<float>::pi;
            auto accAngleY = std::atan(acc_g.y / std::sqrt(pow(acc_g.x, 2) + std::pow(acc_g.z, 2))) * 180.f / juce::MathConstants<float>::pi;
            auto accAngleZ = std::atan(acc_g.z / std::sqrt(pow(acc_g.x, 2) + std::pow(acc_g.y, 2))) * 180.f / juce::MathConstants<float>::pi;

            pitchRollYaw.x = (float)((filterweight * accAngleX) + ((1.0 - filterweight) * gyr_g.x * dt / 1000.0));
            pitchRollYaw.y = (float)((filterweight * accAngleY) + ((1.0 - filterweight) * gyr_g.y * dt / 1000.0));
            pitchRollYaw.y = (float)((filterweight * accAngleZ) + ((1.0 - filterweight) * gyr_g.z * dt / 1000.0));

            dt = 1;
        }

        timestamp = report_buf[1];

        return 0;
    }

    std::vector<float> CenterSticks(std::vector<uint16> vals)
    {
        std::vector<float> s = { 0, 0 };

        for (uint i = 0; i < 2; ++i)
        {
            float diff = vals[i] - stick_cal[2 + i];

            if (std::abs(diff) < deadzone)
            {
                vals[i] = 0;
            }
            else if (diff > 0) // if axis is above center
            {
                s[i] = diff / stick_cal[i];
            }
            else
            {
                s[i] = diff / stick_cal[4 + i];
            }
        }
        return s;
    }

    void SendRumble(std::vector<uint8_t> buf)
    {
        std::vector<uint8_t> report(report_len);

        report[0] = 0x10;
        report[1] = global_count;

        if (global_count == 0xf) global_count = 0;
        else ++global_count;

        report.insert(report.begin() + 2, buf.begin(), buf.end());

        PrintArray(report, DebugType::RUMBLE, std::ios_base::hex);

        if (-1 == hid_write(hid_dev, report.data(), report.size()))
        {
            DebugPrint("Failed to send rumble data", DebugType::COMMS);
        }
    }

    std::vector<uint8_t> Subcommand(uint8_t sc, std::vector<uint8_t> buf, bool print = true)
    {
        std::vector<uint8_t> report(report_len);
        std::vector<uint8_t> response(report_len);

        report.insert(report.begin() + 2, default_buf.begin(), default_buf.end());
        report.insert(report.begin() + 11, buf.begin(), buf.end());

        report[10] = sc;
        report[1] = global_count;
        report[0] = 0x1;

        if (global_count == 0xf) global_count = 0;
        else ++global_count;

        std::stringstream ss;
        if (print)
        {
            ss << "Subcommand 0x" << std::hex << std::setfill('0') << std::setw(2) << sc << " sent" << std::endl;
            DebugPrint(ss.str(), DebugType::COMMS);
            PrintArray(report, DebugType::COMMS, std::ios_base::hex);
        };

        hid_write(hid_dev, report.data(), report.size());

        int res = hid_read_timeout(hid_dev, response.data(), response.size(), 50);

        if (res < 1)
        {
            DebugPrint("No response.", DebugType::COMMS);
        }
        else
        if (print)
        {
            ss.clear();
            ss << "Response ID 0x" << std::hex << std::setfill('0') << std::setw(2) << response[0] << std::endl;
            DebugPrint(ss.str(), DebugType::COMMS);
            PrintArray(response, DebugType::COMMS, std::ios_base::hex);
        }

        return response;
    }

    void dumpCalibrationData()
    {
        // get user calibration data if possible
        std::vector<uint8_t> buf_ = ReadSPI(0x80, (isLeft ? (uint8_t)0x12 : (uint8_t)0x1d), 9);
        bool found = false;
        for (size_t i = 0; i < 9; ++i)
        {
            if (buf_[i] != 0xff)
            {
                DebugPrint("Using user stick calibration data.", DebugType::COMMS);
                found = true;
                break;
            }
        }

        if (!found)
        {
            DebugPrint("Using factory stick calibration data.", DebugType::COMMS);

            // get user calibration data if possible
            buf_ = ReadSPI(0x60, (isLeft ? (uint8_t)0x3d : (uint8_t)0x46), 9);
        }

        stick_cal[isLeft ? 0 : 2] = (((uint16_t)buf_[1] << 8) & 0xF00) + buf_[0];           // X Axis Max above center
        stick_cal[isLeft ? 1 : 3] = (uint16_t)(((uint16_t)buf_[2] << 4) + (buf_[1] >> 4));  // Y Axis Max above center
        stick_cal[isLeft ? 2 : 4] = (((uint16_t)buf_[4] << 8) & 0xF00) + buf_[3];           // X Axis Center
        stick_cal[isLeft ? 3 : 5] = (uint16_t)(((uint16_t)buf_[5] << 4) + (buf_[4] >> 4));  // Y Axis Center
        stick_cal[isLeft ? 4 : 0] = (((uint16_t)buf_[7] << 8) & 0xF00) + buf_[6];           // X Axis Min below center
        stick_cal[isLeft ? 5 : 1] = (uint16_t)(((uint16_t)buf_[8] << 4) + (buf_[7] >> 4));  // Y Axis Min below center

        DebugPrint("Stick calibration data: ", DebugType::COMMS);
        PrintArray(stick_cal, DebugType::COMMS);

        buf_ = ReadSPI(0x60, (isLeft ? (uint8_t)0x86 : (uint8_t)0x98), 16);
        deadzone = (((uint16_t)buf_[4] << 8) & 0xF00) + buf_[3];

        buf_ = ReadSPI(0x80, 0x34, 10);
        gyr_neutral.x = (int16_t)(buf_[0] + ((buf_[1] << 8) & 0xff00));
        gyr_neutral.y = (int16_t)(buf_[2] + ((buf_[3] << 8) & 0xff00));
        gyr_neutral.z = (int16_t)(buf_[4] + ((buf_[5] << 8) & 0xff00));

        DebugPrint("User gyro neutral position: ", DebugType::COMMS);
        PrintArray( std::vector<int16_t>{gyr_neutral.x, gyr_neutral.y, gyr_neutral.z},
                    DebugType::IMU,
                    std::ios_base::hex);

        // This is an extremely messy way of checking to see whether there is user stick calibration data present,
        // but I've seen conflicting user calibration data on blank Joy-Cons. Worth another look eventually.
        if ((gyr_neutral.x + gyr_neutral.y + gyr_neutral.z == -3)
        ||  (std::abs(gyr_neutral.x) > 100)
        ||  (std::abs(gyr_neutral.y) > 100)
        ||  (std::abs(gyr_neutral.z) > 100))
        {
            buf_ = ReadSPI(0x60, 0x29, 10);
            gyr_neutral.x = (int16_t)(buf_[3] + ((buf_[4] << 8) & 0xff00));
            gyr_neutral.y = (int16_t)(buf_[5] + ((buf_[6] << 8) & 0xff00));
            gyr_neutral.z = (int16_t)(buf_[7] + ((buf_[8] << 8) & 0xff00));

            DebugPrint("Factory gyro neutral position: ", DebugType::COMMS);
            PrintArray( std::vector<int16_t>{gyr_neutral.x, gyr_neutral.y, gyr_neutral.z},
                        DebugType::IMU,
                        std::ios_base::hex);
        }
    }

    std::vector<uint8_t> ReadSPI(uint8_t addr1, uint8_t addr2, uint len, bool print = false)
    {
        std::vector<uint8_t> report = { addr2, addr1, 0x00, 0x00, (uint8_t)len };
        std::vector<uint8_t> read_buf(len + 20);

        for (int i = 0; i < 100; ++i)
        {
            read_buf = Subcommand(0x10, report, false);
            if (read_buf[15] == addr2 && read_buf[16] == addr1)
            {
                break;
            }
        }

        read_buf.erase(read_buf.begin(), read_buf.begin() + 20);

        if (print)
        {
            PrintArray(read_buf, DebugType::COMMS, std::ios_base::hex);
        }

        return read_buf;
    }

    template <typename T> void PrintArray(  std::vector<T> v,
                                            DebugType d = DebugType::NONE,
                                            std::ios_base::fmtflags base = 0)
    {
        if (d == DebugType::NONE)
        {
            return;
        }

        std::stringstream ss;
        std::string prefix = "";

        if (base == std::ios_base::hex)
        {
            ss << std::hex << std::setfill('0') << std::setw(sizeof(typename std::vector<T>::value_type));
            prefix = "0x";
        }

        for (auto iter = v.begin(); iter < v.end(); iter++)
        {
            ss << prefix << *iter;
        }

        ss << std::endl;

        DebugPrint(ss.str(), d);
    }
};
