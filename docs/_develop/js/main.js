(function(exportObject, exportName) {

    
    const _setupMenu = function() {
        const threshold = 50;
        const element = $(".bsf-menu");

        const update = function() {
            if(window.scrollY < threshold && !element.hasClass("bsf-transparent"))
                element.addClass("bsf-transparent");
            else if(window.scrollY >= threshold && element.hasClass("bsf-transparent"))
                element.removeClass("bsf-transparent");
        };

        update();
        window.addEventListener("scroll", update);

    }

    exportObject[exportName] = {
        setupMenu: _setupMenu
    }

})(window, "bsf");