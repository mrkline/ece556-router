

$(function() {
    $("<button>Reset</button>").prependTo("html").click(function() {
        $(".background").each(function() { $(this).show(); });
    });
    $(".background").each(function() {
        $(this).prepend("<button>Next</button>");
        var $next = $(this).next();
        var $cur = $(this);
        $("button", this).click(function() {
            $next.show();
            $cur.hide();
        });
        var $prev = $(this).prev();
        $("<button>Prev</button>").prependTo(this).click(function() {
            $prev.show();
            $cur.hide();
        });
    });

});
