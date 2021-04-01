
import {Timer} from "./timer";
import {Utils} from "./utils/utils";
// import {DeviceIMU} from "./imu";
import {Preprocessor} from "./preprocessor";
import Worker from "./glitter.worker";

var BAD_FRAMES_BEFORE_DECIMATE = 30;

export class GlitterDetector {
    constructor(codes, targetFps, source, options) {
        this.codes = codes;
        this.targetFps = targetFps; // FPS
        this.fpsInterval = 1000 / this.targetFps; // ms

        this.source = source;
        this.sourceWidth = this.source.options.width;
        this.sourceHeight = this.source.options.height;

        this.imageDecimate = 1.0;

        this.numBadFrames = 0;

        this.options = {
            printPerformance: false,
            decimateImage: true,
            maxImageDecimationFactor: 3,
            imageDecimationDelta: 0.2,
            rangeThreshold: 20,
            quadSigma: 1.0,
            refineEdges: true,
            minWhiteBlackDiff: 50,
        }
        this.setOptions(options);

        // this.imu = new DeviceIMU();
        this.preprocessor = new Preprocessor(this.sourceWidth, this.sourceHeight);
        this.preprocessor.setKernelSigma(this.options.quadSigma);
        this.worker = new Worker();
    }

    init() {
        this.source.init()
            .then((source) => {
                this.preprocessor.attachElem(source);
                this.onInit(source);
            })
            .catch((err) => {
                console.warn("ERROR: " + err);
            });
    }

    onInit(source) {
        let _this = this;
        function startTick() {
            _this.prev = performance.now();
            _this.timer = new Timer(_this.tick.bind(_this), _this.fpsInterval);
            _this.timer.run();
        }

        this.worker.postMessage({
            type: "init",
            codes: this.codes,
            width: this.sourceWidth,
            height: this.sourceHeight,
            targetFps: this.targetFps,
            options: this.options
        });

        this.worker.onmessage = (e) => {
            const msg = e.data
            switch (msg.type) {
                case "loaded": {
                    // this.imu.init();
                    startTick();
                    const initEvent = new CustomEvent(
                        "onGlitterInit",
                        {detail: {source: source}}
                    );
                    window.dispatchEvent(initEvent);
                    break;
                }
                case "result": {
                    const tagEvent = new CustomEvent(
                        "onGlitterTagsFound",
                        {detail: {tags: msg.tags}}
                    );
                    window.dispatchEvent(tagEvent);

                    if (this.options.printPerformance) {
                        console.log("[performance]", "Detect:", msg.performance);
                    }
                    break;
                }
                case "resize": {
                    this.decimate();
                    break;
                }
            }
        }
    }

    decimate() {
        this.imageDecimate += this.options.imageDecimationDelta;
        this.imageDecimate = Utils.round3(this.imageDecimate);
        var width = this.sourceWidth / this.imageDecimate;
        var height = this.sourceHeight / this.imageDecimate;

        this.preprocessor.resize(width, height);
        this.worker.postMessage({
            type: "resize",
            width: width,
            height: height,
            decimate: this.imageDecimate,
        });

        const calibrateEvent = new CustomEvent(
                "onGlitterCalibrate",
                {detail: {decimationFactor: this.imageDecimate}}
            );
        window.dispatchEvent(calibrateEvent);
    }

    setOptions(options) {
        if (options) {
            this.options = Object.assign(this.options, options);
            this.preprocessor.setKernelSigma(this.options.quadSigma);
        }
    }

    addCode(code) {
        this.worker.postMessage({
            type: "add code",
            code: code
        });
    }

    tick(time) {
        const start = performance.now();
        // console.log(start - this.prev, time);
        this.prev = start;

        const imageData = this.preprocessor.getPixels();

        const mid = performance.now();

        this.worker.postMessage({
            type: "process",
            imagedata: imageData
        }, [imageData.buffer]);

        const end = performance.now();

        if (this.options.printPerformance) {
            console.log("[performance]", "getPixels:", mid-start);
            // console.log("[performance]", "postMessage:", end-mid);
        }

        const tagEvent = new CustomEvent(
            "onGlitterTick",
            {detail: {}}
        );
        window.dispatchEvent(tagEvent);

        if (this.options.decimateImage) {
            if (end-start > this.fpsInterval) {
                this.numBadFrames++;
                if (this.numBadFrames > BAD_FRAMES_BEFORE_DECIMATE &&
                    this.imageDecimate < this.options.maxImageDecimationFactor) {
                    this.numBadFrames = 0;
                    this.decimate();
                }
            }
            else {
                this.numBadFrames = 0;
            }
        }
    }
}
