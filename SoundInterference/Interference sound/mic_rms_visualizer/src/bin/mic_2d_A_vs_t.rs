//! Single plot with overlay:
//! - Main layer: RMS over time (scrolling)
//! - Overlay layer: FFT over frequency (fixed X), transparent

use std::{
    collections::VecDeque,
    sync::mpsc::{self, Receiver, SyncSender},
    thread,
    time::{Duration, Instant},
};

use anyhow::Result;
use cpal::{traits::*, Sample, Stream, StreamConfig};
use eframe::egui;
use egui_plot::{Line, Plot, PlotPoints};
use num_traits::ToPrimitive;
use rustfft::{FftPlanner, num_complex::Complex};

/* ─── Settings ─── */
const SAMPLE_RATE: f32 = 44100.0;
const WINDOW_DURATION: f32 = 0.4;
const FFT_SIZE: usize = 2048;
const MAX_FREQ: f32 = 9000.0;
const Y_MAX: f32 = 0.3;

fn main() {
    let (tx, rx) = mpsc::sync_channel::<(f32, f32, Vec<(f32, f32)>)>(2048);

    thread::spawn(move || {
        if let Err(e) = capture_audio(tx) {
            eprintln!("Audio thread error: {e:?}");
        }
    });

    let app = PlotApp::new(rx);
    eframe::run_native(
        "Overlay: RMS-T + FFT-F",
        eframe::NativeOptions::default(),
        Box::new(|_| Box::new(app)),
    ).unwrap();
}

fn capture_audio(tx: SyncSender<(f32, f32, Vec<(f32, f32)>)>) -> Result<()> {
    let host = cpal::default_host();
    let device = host.default_input_device().expect("No mic");
    let cfg = device.default_input_config()?;
    let sr = cfg.sample_rate().0 as f32;

    let _stream = match cfg.sample_format() {
        cpal::SampleFormat::F32 => run_stream::<f32>(&device, &cfg.into(), sr, tx)?,
        cpal::SampleFormat::I16 => run_stream::<i16>(&device, &cfg.into(), sr, tx)?,
        cpal::SampleFormat::U16 => run_stream::<u16>(&device, &cfg.into(), sr, tx)?,
        _ => anyhow::bail!("Unsupported sample format"),
    };

    loop { thread::sleep(Duration::from_secs(1)); }
}

fn run_stream<T>(
    device: &cpal::Device,
    config: &StreamConfig,
    sr: f32,
    tx: SyncSender<(f32, f32, Vec<(f32, f32)>)>
) -> Result<Stream>
where T: Sample + ToPrimitive + Send + 'static + cpal::SizedSample {
    let ch = config.channels as usize;
    let mut planner = FftPlanner::new();
    let fft = planner.plan_fft_forward(FFT_SIZE);
    let mut fft_buf: VecDeque<f32> = VecDeque::with_capacity(FFT_SIZE);
    let start = Instant::now();

    let stream = device.build_input_stream(
        config,
        move |data: &[T], _| {
            for frame in data.chunks(ch) {
                let sample: f32 = frame.iter().filter_map(|v| v.to_f32()).sum::<f32>() / ch as f32;
                if fft_buf.len() == FFT_SIZE { fft_buf.pop_front(); }
                fft_buf.push_back(sample);

                if fft_buf.len() == FFT_SIZE {
                    let t = start.elapsed().as_secs_f32();
                    let rms = (fft_buf.iter().map(|x| x*x).sum::<f32>() / fft_buf.len() as f32).sqrt();

                    let mut input: Vec<Complex<f32>> = fft_buf
                        .iter().map(|&x| Complex{re:x, im:0.0}).collect();
                    fft.process(&mut input);

                    let bin_w = sr / FFT_SIZE as f32;
                    let spectrum: Vec<(f32, f32)> = input.iter().enumerate()
                        .take_while(|(i,_)| *i as f32 * bin_w <= MAX_FREQ)
                        .map(|(i, c)| (i as f32 * bin_w, c.norm() / FFT_SIZE as f32))
                        .collect();

                    let _ = tx.try_send((t, rms, spectrum));
                }
            }
        },
        |err| eprintln!("Stream error: {err:?}"),
        None,
    )?;
    stream.play()?;
    Ok(stream)
}

struct PlotApp {
    rx: Receiver<(f32, f32, Vec<(f32, f32)>)>,
    rms_buf: VecDeque<(f32, f32)>,
    last_spectrum: Vec<(f32, f32)>,
}

impl PlotApp {
    fn new(rx: Receiver<(f32, f32, Vec<(f32, f32)>)>) -> Self {
        Self { rx, rms_buf: VecDeque::new(), last_spectrum: vec![] }
    }
}

impl eframe::App for PlotApp {
    fn update(&mut self, ctx: &egui::Context, _: &mut eframe::Frame) {
        while let Ok((t, a, fft)) = self.rx.try_recv() {
            self.rms_buf.push_back((t, a));
            self.last_spectrum = fft;
        }

        let t_max = self.rms_buf.back().map_or(0.0, |(t, _)| *t);
        let t_min = t_max - WINDOW_DURATION;
        while self.rms_buf.front().map_or(false, |(t, _)| *t < t_min) {
            self.rms_buf.pop_front();
        }

        let rms_pts: Vec<[f64; 2]> = self.rms_buf.iter().map(|(t,a)| [*t as f64, *a as f64]).collect();
        let fft_pts: Vec<[f64; 2]> = self.last_spectrum.iter().map(|(f,a)| [*f as f64, *a as f64]).collect();

        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("Overlay Plot: Blue = RMS(t), Red = FFT(f)");
            Plot::new("rms_fft_overlay")
                .include_y(0.0).include_y(Y_MAX)
                .include_x(t_min as f64).include_x(t_max as f64)
                .view_aspect(2.0)
                .show(ui, |plot| {
                    plot.line(Line::new(rms_pts).color(egui::Color32::BLUE).name("RMS"));
                    plot.line(Line::new(fft_pts).color(egui::Color32::RED).name("FFT").highlight(true));
                });
        });

        ctx.request_repaint();
    }
}
