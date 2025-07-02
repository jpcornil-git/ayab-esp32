// Initialize page with home menu item
function menuGo(item) {
    setTimeout(() => {
        const initialLink = document.querySelector(`.menu-bar ul li a[data-content=${item}]`);
        if (initialLink) {
            initialLink.click();
        }
    }, 0);
}   

// Collect id of selected rows in a table
function getSelectedRowIds(id) {
    const checkboxes = document.querySelectorAll(`.${id}:checked`);
    const selectedIds = Array.from(checkboxes).map(checkbox => checkbox.dataset.id);
    return selectedIds;
}
