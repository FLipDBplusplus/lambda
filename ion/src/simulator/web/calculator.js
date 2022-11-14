function Calculator(emModule) {
  /* emModule is an optional parameter.
   * In addition to the ones supported by Emscripten, here are the values that
   * this object can have:
   *  - element: a DOM element containing a copy of 'calculator.html' as
   *    generated by 'layout.py'
   *  - mirrorCanvas: a DOM element where the main canvas will be mirrored
   */

  // Configure emModule
  var emModule = (typeof emModule === 'undefined') ? {} : emModule;
  var calculatorElement = emModule.element || document.querySelector('.calculator');
  var mainCanvas = calculatorElement.querySelector("canvas");
  if (typeof emModule.mirrorCanvas === 'undefined') {
    /* If emModule.mirrorCanvas is defined as null, don't do anything */
    emModule.mirrorCanvas = document.querySelector('canvas.calculator-mirror');
  }
  var mirrorCanvasContext = emModule.mirrorCanvas ? emModule.mirrorCanvas.getContext('2d') : null;
  var defaultModule = {
    canvas: mainCanvas,
    arguments: [
      '--language',
      document.documentElement.lang || window.navigator.language.split('-')[0]
    ],
    onEpsilonIdle: function() {
      calculatorElement.classList.remove('loading');
    },
    takeScreenshot: function(e) {
      if (e.altKey) {
        this.copyScreenshot();
      } else {
        this.downloadScreenshot();
      }
    },
    copyScreenshot: function() {
      this.screenshot(function(blob){
        let data = [new ClipboardItem({[blob.type]: blob})];
        navigator.clipboard.write(data);
      });
    },
    downloadScreenshot: function() {
      this.screenshot(function(blob){
        var url = URL.createObjectURL(blob);
        console.log(blob, url);
        var link = document.createElement('a');
        link.download = 'screenshot.png';
        link.href = url;
        link.click();
        URL.revokeObjectURL(url);
      });
    },
    screenshot: function(callback) {
      this._IonDisplayPrepareForScreenshot();
      return mainCanvas.toBlob(callback);
    }
  };
  if (mirrorCanvasContext) {
    defaultModule.onDisplayRefresh = function() {
      mirrorCanvasContext.drawImage(mainCanvas, 0, 0);
    }
  }
  for (var attrname in defaultModule) {
    if (!emModule.hasOwnProperty(attrname)) {
      emModule[attrname] = defaultModule[attrname];
    }
  }

  // Load and run Epsilon
  Epsilon(emModule);

  /* Install event handlers
   * This needs to be done after loading Epsilon, otherwise the _IonSimulator*
   * functions haven't been defined just yet. */
  function eventHandler(keyHandler) {
    return function(ev) {
      var key = this.getAttribute('data-key');
      keyHandler(key);
      /* Always prevent default action of event.
       * This will prevent the browser from delaying that event. Indeed the
       * browser would otherwise try to see if that event could have any other
       * meaning (e.g. a click) and might delay it as a result.*/
      ev.preventDefault();
    };
  }
  var downHandler = eventHandler(emModule._IonSimulatorKeyboardKeyDown);
  var upHandler = eventHandler(emModule._IonSimulatorKeyboardKeyUp);

  calculatorElement.querySelectorAll('span').forEach(function(span){
    /* We use pointer events to handle both touch and mouse events */
    span.addEventListener('pointerdown', downHandler);
    span.addEventListener('pointerup', upHandler);
  });
}

if (typeof exports === 'object' && typeof module === 'object') {
  module.exports = Calculator;
}
