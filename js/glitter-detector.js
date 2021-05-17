
import {Timer} from "./timer";
import {Utils} from "./utils/utils";
// import {DeviceIMU} from "./imu";
import {Preprocessor} from "./preprocessor";
import GlitterWorker from "./glitter.worker";

var BAD_FRAMES_BEFORE_DECIMATE = 20;

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
            rangeThreshold: 15,
            quadSigma: 0.2,
            minWhiteBlackDiff: 50,
        }
        this.setOptions(options);

        // this.imu = new DeviceIMU();
        this.preprocessor = new Preprocessor(this.sourceWidth, this.sourceHeight);
        this.preprocessor.setKernelSigma(this.options.quadSigma);
    }

    init() {
        this.source.init()
            .then((source) => {
                this.onInit(source);
            })
            .catch((err) => {
                console.warn("ERROR: " + err);
            });
    }

    async onInit(source) {
        let _this = this;
        function startTick() {
            _this.prev = Date.now();
            _this.timer = new Timer(_this.tick.bind(_this), _this.fpsInterval);
            _this.timer.run();
        }

        this.preprocessor.attachElem(source);
        this.worker = await new GlitterWorker();
        await this.worker.init(this.codes, this.sourceWidth, this.sourceHeight, this.options);

        startTick();
        const initEvent = new CustomEvent(
            "onGlitterInit",
            {detail: {source: source}}
        );
        window.dispatchEvent(initEvent);
    }

    decimate() {
        this.imageDecimate += this.options.imageDecimationDelta;
        this.imageDecimate = Utils.round3(this.imageDecimate);
        var width = this.sourceWidth / this.imageDecimate;
        var height = this.sourceHeight / this.imageDecimate;

        this.preprocessor.resize(width, height);
        this.worker.resize(width, height, this.imageDecimate);

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
        this.worker.addCode(code);
    }

    async tick() {
        const start = Date.now();
        // console.log(start - this.prev, this.timer.getError());
        this.prev = start;

        const imageData = this.preprocessor.getPixels();

        this.worker.process(imageData)
            .then((tags) => {
                const tagEvent = new CustomEvent(
                    "onGlitterTagsFound",
                    {detail: {tags: tags}}
                );
                window.dispatchEvent(tagEvent);
            });

        const end = Date.now();

        if (this.options.printPerformance) {
            console.log("[performance]", "Get Pixels:", end-start);
        }

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
