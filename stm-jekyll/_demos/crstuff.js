window.onload = function(){
  let element = document.getElementById("details");
  element.innerHTML ="Klicke auf eine Region f&uuml;r mehr Details.";
}
function showTooltip(evt, tooltip) {
  if (tooltip) {
    let element = document.getElementById("tooltip");
    element.innerHTML = tooltip;
    element.style.display = "block";
    element.style.left = evt.pageX + 10 + 'px';
    element.style.top = evt.pageY + 10 + 'px';
  }
}
function showDescription(e, id) {
  e.preventDefault();
  if (id) {
    let element = document.getElementById("details");
    let desc = document.getElementById(id).children[1].innerHTML;
    element.innerHTML = desc;
    element.style.display = "block";
  }
  return false;
}

function hideTooltip() {
  var element = document.getElementById("tooltip");
  element.style.display = "none";
}
