from django.conf import settings
from django.conf.urls.static import static
from django.contrib import admin
from django.urls import path, include

urlpatterns = [
    path('admin/', admin.site.urls),
    path('ultrasonic/', include('ultrasonic.urls')),
    path('api/', include('image_classifier.urls')),
] + static(settings.MEDIA_URL, document_root=settings.MEDIA_ROOT)
