import {GlitterModule} from './glitter-module';

var glitterModule = null;

export function init(codes, width, height, options) {
    glitterModule = new GlitterModule(
        codes,
        width,
        height,
        options
    );
}

export function addCode(code) {
    if (glitterModule) {
        glitterModule.addCode(code);
    }
}

export function resize(width, height, decimate) {
    if (glitterModule) {
        glitterModule.resize(width, height);
        glitterModule.setQuadDecimate(decimate);
    }
}

export function process(grayScaleImg) {
    if (glitterModule) {
        const start = Date.now();

        glitterModule.saveGrayscale(grayScaleImg);
        const tags = glitterModule.detectTags();

        const end = Date.now();

        if (glitterModule.options.printPerformance) {
            console.log("[performance]", "Detect:", end-start);
        }

        return tags;
    }
}
